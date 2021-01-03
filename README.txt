
    Welcome to PSPIRC


1. INTRODUCTION
   ------------

  PSPIRC is a basic IRC client for the PSP. It is based on the IRC engine
  written by Danzel.

  Many thanks to Danzel for his Virtual kerboard and his IRC stuff, 
  and to all PSPSDK developpers.

  Thanks to Snaap06 for his work on previous eboot icons.

  Special thanks to HookBott for his awesome work on graphics and eboot icons 
  on previous releases.

  Many thanks to Delight1 for her help, support, beta testing and for her
  very nice eboot icons and graphics on this release !

  Big thanks to NeXuS for his very nice music ...
  ( see http://www.jamendo.com/en/artist/nexus )

  This software is distributed under GNU V2 License, see GPLV2.txt file 
  for all details and information about it.

2. INSTALLATION
   ------------

  Unzip the zip file, and copy the content of the directory fw1.5 or fw3.x
  (depending of the version of your firmware) on the psp/game, or psp/game380
  if you use custom firmware 3.80-m33 or 3.90-m33

  It has been developped on linux using Firmware 3.90-M33 on both SLIM & FAT.

  For any comments or questions on this version, please visit 
  http://zx81.zx81.free.fr, http://www.dcemu.co.uk, or 
  http://pspupdates.qj.net

  If you need any help about irc and psp subject then feel free to go to 
  ion.ircdotnet.net and irc.pedo-bear.com (#leak #ot and #main)


3. CONTROL
   ------------

3.1 - Virtual keyboard

  In the console window, press Start to open/close the 
  On-Screen keyboard

  The On-Screen Keyboard of "Danzel" and "Jeff Chen"

  Use Analog stick to choose one of the 9 squares, and
  use Triangle, Square, Cross and Circle to choose one
  of the 4 letters of the highlighted square.

3.2 - IR keyboard

  You can also use IR keyboard. Edit the pspirkeyb.ini file
  to specify your IR keyboard model, and modify eventually
  layout keyboard files in the keymap directory.

  The following mapping is done :

  IR-keyboard   PSP

  Cursor        Digital Pad

  Tab           Start
  Ctrl-W        Start

  Escape        Select
  Ctrl-Q        Select

  Ctrl-E        Triangle
  Ctrl-X        Cross
  Ctrl-S        Square
  Ctrl-F        Circle
  Ctrl-Z        L-trigger
  Ctrl-C        R-trigger

  Ctrl-L        Clear


4. HOW DOES-IT WORK ?
   ------------

4.1 - Create a connection with host, user, channel ... :
 
  Host: name of the IRC server
  Port: by default it's set to 6667, which is the standard IRC port but 
        you can change it.

  User    : user name for this connection 
  Nickname: nickname for this connection 
  Realname: realname for this connection 
  Password: password (optional) for this connection
  Channel : channel name for this connection 
  Script  : the startup script (optional) for this connection

  The channel name should begin with a # for example:
    #psp
  You can specify more than one channel:
    #psp#etc

  All those parameters can be easily entered using the Virtual Keyboard (Press
  Start to display the keyboard).

  You can use the CLEAR key in the virtual keyboard (or CTRL-L with IR keyboard)
  to clear an input field.

  You can save your parameters in a binary encoded file if you want using the
  "Save profiles" menu.

  You can specify a startup script that will be executed just after the 
  connection, for example to join channels and set modes automatically.
  (Use then JOIN and MODE commands in your startup script).
 
  Those scripts should be stored in the 'run' directory.

4.2 - Connect to the IRC server

  Then you can connect to the host you've specified using the
  "Connect" menu.

  If everything is going right, you should see a console with
  a prompt few seconds after.

4.3 - Enter commands in the console window

  In the console window you can use the Virtual Keyboard to
  enter text or IRC commands.

  The D-pad is mapped as follows :

  When the virtual keyboard is off :

    Up    : Toggle Begin/End line
    Down  : <ENTER>
    Left  : Left  (or command left, see below)
    Right : Right (or command right, see below)


  When the virtual keyboard is on :

    Up    : History up
    Down  : History down
    Left  : Left  (or command left, see below)
    Right : Right (or command right, see below)

  Using Analog pad you can scroll the console window.  Using the start button
  you can use the Virtual keyboard. Using LTrigger and RTrigger keys you can
  navigate through the different opened channels.

  Press the Select key to enter in the Console menu.  You will then be able to
  choose IRC commands, or common words in a list. 

  Use the Virtual Keyboard to specify the first letter of the command or the
  word (using letters panel).

  If you press the same letter several time then all commands beginning with
  this letter will be displayed sequentially.

  All those commands are written in file command.txt that can be edited using
  the built-in editor (Edit Command).  You can also use any ASCII-text editor on
  your PC, and add your own custom commands.

  It might be better to have all commands alphabetically sorted !

  When you enter commands you can use the Users menu to paste a user name
  present in the current channel.  In the same maner, you can enter in the
  Channels or the tabs menu to paste the name of a channel you have joined.

  If you begin the line with a '/' (meaning you want to enter an IRC command)
  then using the left and right cursor key you can choose the command you
  want (all commands present in the command.txt will be displayed). 
  When you have selected the command you want, then you can press the space
  key, or CROSS to enter next command parameters.

  If the virtual keyboard is off, you can use the left key and press then the
  Rtrigger key to go directly to the beginning of the input line. If you use
  the right key and press then the Rtrigger key you can go directly to the end
  of the line.

4.4 - Change text color

  You can insert text color codes with the "Paste color" menu.  
  It will display a "strange" character with a color number in the input field
  (the color is a number between 0 and 15).  
  You can specify a couple background, foreground color using two numbers 
  separated by a comma.

  In the Danzeff keyboard you can use the 'color' item to begin or end a text
  color command.

  If you want to use a foreground color permanently then you'd better use the
  "Text" menu to specify the color you want.

4.5 - Search channels 

  The "Search channels" menu will let you display / search all channels 
  available on the server you are connected on. The channel list can
  be huge, so you can limit their numbers by given the minimum connected 
  users, or using keyword and/or regular expressions.

  You may need to refresh the list, if you want to see newly created 
  channels.

  You need to press the Triangle key to toggle between the search 
  options (i.e: minimum users and string match), and the channel list.

  If you want to join a channel, you have just to select it the list
  and then to press X key to join.

4.6 - Users 
 
  The "Users" menu will let you display nickname, username, hostname of 
  users connected to the current channel, and also the permissions 
  they have.

  o : means operator
  h : means half-operator
  v : means voice on

  The "Users" menu can be used to write private message to a given user.
  If you press the X key, it opens a new tab with this private discussion. 

  You can also press the Triangle Key to enter in the Operator/Modes menu.

  In the Modes/Operator menu, you can change permissions of given users.
  This menu can also be used to kick or ban any users (if you have the 
  proper privileges).

  The who and WhoIs menu let you run a Who or WhoIs command on the 
  given user.

4.7 - Channel tabs

  If you want to display current opened channels then you can use 
  this menu.

4.8 - Close tabs

  It lets you close the tab you want. A '+' in before the tab name
  means there are unread text on this tab.

4.9 - Run script

  You can write your own script files, and put them in the 'run' 
  directory.  This menu let you choose the file you want to run.

4.10 - Settings

  The settings menu let you change colors and some other options.
  If you want a distinct color for each user in a channel, then
  activate the 'Multi color' option.

  You can also prefix all posts by a time stamp.
  

5. Save snapshots and log files
   ------------

  You can save snapshot in PNG format using the snapshot option.
  Files would be saved in the scr directory.

  You can also save the IRC console window as a text file.
  Files would be saved in the log directory.
  

6. COMPILATION
   ------------

  It has been developped under Linux using gcc with PSPSDK. 
  To rebuild the homebrew run the Makefile in the src archive.


  Enjoy,
              Zx
