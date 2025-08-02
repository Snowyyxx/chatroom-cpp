// commands.h
#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>

// Call once at server startup
void Commands_Init();

// Try to handle a leading-‘/’ command from client `sender_id`.
// Returns true if handled (and no further broadcast is needed).
bool Commands_Handle(const std::string& input, int sender_id);

#endif // COMMANDS_H
