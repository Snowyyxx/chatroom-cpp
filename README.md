This project is built on top of an existing open-source chatroom system. My contributions include:

💬 Command System Integration — Added support for slash commands:

/help – Display available commands

/users – Show a list of online users

/rename <new_name> – Change username dynamically

/msg <user> <message> – Send private messages

/color <0–5> – Customize your chat text color

🧵 Modularization — Refactored command logic into a separate commands.cpp and commands.h for cleaner architecture

🌈 Color Customization — Implemented client-side support for real-time color changes via command

🔐 Thread Safety Enhancements — Wrapped access to shared resources (like client list and output) with mutex locks to prevent race conditions

🚪 Graceful Disconnects — Added proper cleanup for user exit (via /quit or Ctrl+C) to avoid dangling threads or broken pipes

📃 Improved Output Formatting — Polished user-facing messages and prompts for better readability and user experience

📸 Added UI Preview — Included a screenshot to visualize the interface

