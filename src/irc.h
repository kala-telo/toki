#ifndef IRC_H
#define IRC_H
#include <stddef.h>

typedef struct {
    char *data;
    size_t len, cap;
} StringBuilder;

typedef struct {
    StringBuilder sender;
    StringBuilder text;
    // TODO add message types (ie normal message, error, server info, etc)
} Message;

typedef struct {
    Message *data;
    size_t len, cap;
} Messages;

typedef struct {
    StringBuilder name;
    StringBuilder topic;
    Messages messages;
    bool joined;
} Channel;

typedef struct {
    Channel *data;
    size_t len, cap;
} Channels;

void irc_proccess(void);
void irc_close(void);
void irc_connect(char *server, char *username);
void irc_send_message(char *message, char *channel);
void irc_join_channel(char *channel);

extern Messages system_messages;
extern Channels channels;
extern int current_channel;

#endif
