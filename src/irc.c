#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>

#include "da.h"
#include "irc.h"

typedef enum {
    RPL_WELCOME        =   1,
    RPL_YOURHOST       =   2,
    RPL_CREATED        =   3,
    RPL_MYINFO         =   4,
    RPL_ISUPPORT       =   5,
    RPL_LUSERCLIENT    = 251,
    RPL_LUSERCHANNELS  = 254,
    RPL_LUSERME        = 255,
    RPL_LOCALUSERS     = 265,
    RPL_NETUSERS       = 266,
    RPL_STATSCONN      = 250,
    RPL_LISTSTART      = 321,
    RPL_LIST           = 322,
    RPL_LISTEND        = 323,
    RPL_TOPIC          = 332,
    RPL_TOPICSETBY     = 333,
    RPL_NAMREPLY       = 353,
    RPL_ENDOFNAMES     = 366,
    ERR_NOMOTD         = 422,
} IrcReply;

// might be a good idea to just pass it as an argument
extern int server_fd;

static struct {
    char *data;
    size_t len, cap, pos;
} lex;

static void free_string_builder(StringBuilder *sb) {
    // sb->cap = 0 means that it is either empty or statically allocated
    if (sb->data != NULL && sb->cap != 0) {
        free(sb->data);
        sb->data = NULL;
    }
}

static void free_messages(Messages *msgs) {
    for (size_t j = 0; j < msgs->len; j++) {
        Message *msg = &msgs->data[j];
        free_string_builder(&msg->sender);
        free_string_builder(&msg->text);
    }
}

static void free_channel(Channel *channel) {
    free_messages(&channel->messages);
    free_string_builder(&channel->name);
    free_string_builder(&channel->topic);
}

static inline bool str_equal(StringBuilder s1, StringBuilder s2) {
    if (s1.len != s2.len)
        return false;
    return memcmp(s1.data, s2.data, s1.len) == 0;
}

static Channel *find_channel(StringBuilder name) {
    for (size_t i = 0; i < channels.len; i++) {
        if (str_equal(name, channels.data[i].name))
            return &channels.data[i];
    }
    return NULL;
}

static void irc_listen(void) {
    char buf[65535] = {0};
    ssize_t len = read(server_fd, buf, sizeof(buf));
    if (len == -1) {
        if (errno == EAGAIN)
            return;
        perror("irc_listen");
        exit(1);
    }
    if (len == 0) {
        close(server_fd);
        server_fd = -1;
        return;
    }
    da_append_many(lex, buf, len);
    if (len == sizeof(buf)) {
        irc_listen();
    }
}

static char current_char(void) {
    return lex.data[lex.pos];
}

static char eat_char(void) {
    if (lex.len <= lex.pos) {
        lex.pos = 0;
        lex.len = 0;
    }
    irc_listen();
    if (lex.len <= lex.pos)
        return '\0';
    return lex.data[lex.pos++];
}

static char escape_char(char c) {
    unsigned char uc = c;
    return 31 < uc && uc < 128 ? uc : '.';
}

static void skip_char(char c) {
    char d;
    if ((d = eat_char()) != c) {
        printf("Expected %c[%d] but got %c[%d]\n", escape_char(c), c, escape_char(d), d);
        abort();
    }
}

static void skip_string(char *c) {
    while (*c != '\0') {
        skip_char(*c);
        c++;
    }
}

static void collect_until(StringBuilder *sb, char until) {
    while (current_char() != until) {
        da_append(*sb, eat_char());
    }
}

static int skip_until(char until) {
    int n = 0;
    while (true) {
        char c = eat_char();
        if (c == '\0' || c == until)
            break;
        n++;
    }
    return n;
}

static int parse_3digit(void) {
    int n = 0;
    int pows[3] = {100, 10, 1};
    for (int i = 0; i < 3; i++) {
        char c = eat_char();
        if (!isdigit(c)) {
            abort();
        }
        n += (c-'0')*pows[i];
    }
    return n;
}

static int parse_number(void) {
    char num[10];
    size_t len = 0;
    while (isdigit(current_char())) {
        if (len >= 10) {
            printf("Int to be parsed is larger than i32 can store\n");
            abort();
        }
        num[len] = eat_char();
        len++;
    }
    int n = 0;
    for (size_t i = 1; i < len+1; i++) {
        n *= 10;
        n += num[len-i]-'0';
    }
    return n;
}

void irc_connect(StringBuilder *server, StringBuilder *username) {
    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char *server_cstr = malloc(server->len+1);
    server_cstr[server->len] = '\0';
    memcpy(server_cstr, server->data, server->len);
    if (getaddrinfo(server_cstr, "6667", &hints, &res) != 0)
        abort();
    free(server_cstr);
    for (struct addrinfo *r = res; r; r = r->ai_next) {
        server_fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
        if (server_fd == -1)
            continue;
        if (connect(server_fd, r->ai_addr, r->ai_addrlen) == 0)
            break;
        close(server_fd);
        server_fd = -1;
    }
    freeaddrinfo(res);
    // TODO add error message instead of crashing
    if (server_fd == -1)
        abort();
    dprintf(server_fd, "NICK %.*s\r\n", (int)username->len, username->data);
    dprintf(server_fd, "USER %.*s * * :%.*s\r\n",
            (int)username->len, username->data,
            (int)username->len, username->data);
    dprintf(server_fd, "LIST\r\n");
    if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0)
        abort();
}

static void parse_int_message(StringBuilder from, IrcReply code) {
    Message msg = {0};
    msg.sender = from;
    Messages *messages = current_channel == -1
                             ? &system_messages
                             : &channels.data[current_channel].messages;
    bool free_from = true;
    switch (code) {
    case RPL_WELCOME: {
        // skip username
        skip_until(' ');
        skip_char(':');
        collect_until(&msg.text, '\r');
        skip_string("\r\n");
        da_append(*messages, msg);
        free_from = false;
    } break;
    case RPL_ISUPPORT:
        // username
        skip_until(' ');
        // til end
        skip_until('\r');
        skip_char('\n');
        break;
    case RPL_CREATED: {
        // skip username
        skip_until(' ');
        skip_char(':');
        collect_until(&msg.text, '\r');
        skip_string("\r\n");
        da_append(*messages, msg);
        free_from = false;
    } break;
    case RPL_MYINFO: {
        // username
        skip_until(' ');
        // server version
        skip_until(' ');
        // available umodes
        skip_until(' ');
        // available cmodes
        skip_until(' ');
        // cmodes with param
        skip_until('\r');
        skip_char('\n');
    } break;
    case RPL_YOURHOST: {
        // skip username
        skip_until(' ');
        skip_char(':');
        collect_until(&msg.text, '\r');
        skip_string("\r\n");
        da_append(*messages, msg);
        free_from = false;
    } break;
    case RPL_LUSERCLIENT: {
        // skip username
        skip_until(' ');
        skip_char(':');
        collect_until(&msg.text, '\r');
        skip_string("\r\n");
        da_append(*messages, msg);
        free_from = false;
    } break;
    case RPL_LUSERCHANNELS: {
        // skip username
        skip_until(' ');

        // TODO do something with it
        (void)parse_number();
        skip_string(" :");
        collect_until(&msg.text, '\r');
        skip_string("\r\n");
        da_append(*messages, msg);
        free_from = false;
    } break;
    case RPL_LUSERME: {
        // skip username
        skip_until(' ');
        skip_char(':');
        collect_until(&msg.text, '\r');
        skip_string("\r\n");
        da_append(*messages, msg);
        free_from = false;
    } break;
    case RPL_LOCALUSERS: {
        // skip username
        skip_until(' ');
        // count
        skip_until(' ');
        // max
        skip_until(' ');
        // till end
        skip_until('\r');
        skip_char('\n');
    } break;
    case RPL_NETUSERS: {
        // skip username
        skip_until(' ');
        // count
        skip_until(' ');
        // max
        skip_until(' ');
        // till end
        skip_until('\r');
        skip_char('\n');
    } break;
    case RPL_STATSCONN: {
        // skip username
        skip_until(' ');
        // till end
        skip_until('\r');
        skip_char('\n');
    } break;
    case RPL_LISTSTART: {
        // NOTE: afaik, IRC only lists channels, hence we can just skip this
        // message, as well as RPL_LISTEND
        skip_until('\r');
        skip_char('\n');
    } break;
    case RPL_LIST: {
        // skip username
        skip_until(' ');
        da_append_empty(channels);
        collect_until(&da_last(channels).name, ' ');
        skip_char(' ');
        da_append(da_last(channels).name, '\0');
        da_last(channels).name.len--;
        // (maybe) TODO: topic
        skip_until('\r');
        skip_char('\n');
    } break;
    case RPL_LISTEND: {
        // NOTE: see RPL_LISTSTART
        skip_until('\r');
        skip_char('\n');
    } break;
    case RPL_TOPIC: {
        skip_until(' ');
        StringBuilder channel_name = {0};
        collect_until(&channel_name, ' ');
        skip_string(" :");
        Channel *channel = find_channel(channel_name);
        collect_until(&channel->topic, '\r');
        free_string_builder(&channel_name);
        skip_string("\r\n");
    } break;
    case RPL_TOPICSETBY: {
        // // the client has no way to display it yet
        // skip_until(' ');
        // StringBuilder channel = {0};
        // collect_until(&channel, ' ');
        // StringBuilder by = {0};
        // collect_until(&by, ' ');
        skip_until('\r');
        skip_char('\n');
    } break;
    case RPL_NAMREPLY: {
        // TODO
        // // StringBuilder channel_name = {0};
        // // collect_until(&channel_name, ' ');
        skip_until('\r');
        skip_char('\n');
    } break;
    case RPL_ENDOFNAMES:
        skip_until('\r');
        skip_char('\n');
        break;
    case ERR_NOMOTD: {
        // skip username
        skip_until(' ');
        // till end
        skip_until('\r');
        skip_char('\n');
    } break;
    default:
        printf("Unimplemented code: %02d\n", code);
        abort();
    }
    if (free_from)
        free_string_builder(&from);
}

#define SB(s) (StringBuilder){.data = (s), .len = sizeof(s)-1}

static void parse_str_message(StringBuilder from) {
    StringBuilder command = {0};
    collect_until(&command, ' ');
    skip_char(' ');
    Message msg = {0};
    msg.sender = from;
    if (str_equal(command, SB("JOIN"))) {
        StringBuilder channel_name = {0};
        skip_char(':');
        collect_until(&channel_name, '\r');
        skip_string("\r\n");
        msg.text = SB("joined");
        Channel *channel = find_channel(channel_name);
        free_string_builder(&channel_name);
        da_append(channel->messages, msg);
    } else if (str_equal(command, SB("PRIVMSG"))) {
        // TODO multiple targets
        StringBuilder to = {0};
        collect_until(&to, ' ');
        Messages *where = NULL;
        if (to.data[0] == '#') {
            Channel *channel = find_channel(to);
            where = &channel->messages;
        } else {
            // TODO
            abort();
        }
        skip_string(" :");
        collect_until(&msg.text, '\r');
        skip_string("\r\n");
        da_append(*where, msg);
    } else {
        printf("Unimplemented command: %.*s\n", (int)command.len, command.data);
        abort();
    }
    free_string_builder(&command);
}

void irc_proccess(void) {
    irc_listen();
    while (lex.len-lex.pos > 0) {
        if (current_char() == 'P') {
            skip_string("PING ");
            StringBuilder origin = {0};
            collect_until(&origin, '\r');
            skip_string("\r\n");
            dprintf(server_fd, "PONG %.*s\r\n", (int)origin.len, origin.data);
            free_string_builder(&origin);
            continue;
        }
        skip_char(':');
        StringBuilder from = {0};
        collect_until(&from, ' ');
        skip_char(' ');
        if (isdigit(current_char())) {
            IrcReply code = parse_3digit();
            skip_char(' ');
            parse_int_message(from, code);
        } else {
            parse_str_message(from);
        }
    }
}

void irc_destroy(void) {
    for (size_t i = 0; i < channels.len; i++) {
        Channel *channel = &channels.data[i];
        free_channel(channel);
    }
   free_messages(&system_messages);
}

void irc_close(void) {
    if (server_fd != -1)
        close(server_fd);
}

void irc_join_channel(StringBuilder *channel) {
    dprintf(server_fd, "JOIN %.*s\r\n", (int)channel->len, channel->data);
}

void irc_send_message(StringBuilder *message, StringBuilder *channel) {
    dprintf(server_fd, "PRIVMSG %.*s :%.*s\r\n",
            (int)channel->len, channel->data,
            (int)message->len, message->data);
}
