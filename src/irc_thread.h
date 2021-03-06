#ifndef INCLUDED_PROTOCOL_IRC_H
#define INCLUDED_PROTOCOL_IRC_H

#include <pspkernel.h>

#ifdef __cplusplus

#include <list>
#include <map>
#include <vector>
#include <string>
#include "irc_util.h"
#include <netinet/in.h>
using namespace std;

class irc;

//Status of irc connection
enum connectionStatus {
	CS_OFFLINE=1,		//Not connected at all
	CS_CONNECTING=2,	//Attempting to connect
	CS_IDENTIFYING=3,	//connected to server, registering nick and waiting for okay message
	CS_CONNECTED=4,		//Really connected to server, do what you want
};

//Types of message that a server message could contain
enum serverMessageType {
	SM_IRC_UNK=1,        //unknown line from the irc class
	SM_IRC_DETAILS=2,    //message from the irc class on status etc
	SM_IRC_ERROR=3,      //error message from the irc class
	SM_GENERAL_TEXT=4,   //general text from the server, MOTD etc
	SM_NOTICE=5,         //NOTICE message from the server
	SM_RENAME=6,         //Message format "oldnick newnick", this is a nick change. FIXME
	SM_NONFATAL_ERROR=7, //an error message of some kind, not fatal (your last command probably failed)
	SM_NICK_TAKEN=8,     //The nick you are trying to use is already taken. (msg = attempted nick)
	SM_QUIT=9,           //Message format "nick reason", reason may contain spaces FIXME
	SM_INVITE=10,        //Message format "nick channel" FIXME
	SM_JOIN_CHAN=11,     //You have joined a channel. Message: "#channel"
	SM_LEFT_CHAN=12,     //You have left a channel. Message: "#channel"
  SM_WHO=13,           //Who list entry
	SM_WHO_DONE=14,      //Got who list for given channel/person. Message: "#channel" / "nick????"
	SM_UNKNOWN=15,       //I DUNO LOL
	
	SM_WHOIS_IP=16,      //users ip from a whois, NOT IMPLEMENTED
	SM_WHOIS_REALNAME=17,//users realname from a whois "nick realname"
	SM_WHOIS_SERVER=18,  //users server "name" (not ip) from a whois "nick servername"
	SM_WHOIS_AWAY=19,    //users away message (if any) from a whois "nick message"
	SM_WHOIS_END=20,     //end of whois responses about the given user 

	SM_LIST_BEGIN=21,    //Begin channel list
	SM_LIST_CHAN=22,     //Add channel list item
	SM_LIST_END=23,      //End channel list
  
  SM_BANLIST=24,       //New ban list entry
  SM_BANLIST_DONE=25,  //End ban list
};

enum channelChangeType {
	CC_TOPIC=1,	//Message format "nick topic", topic may contain spaces FIXME
	CC_JOIN=2,
	CC_PART=3,
	CC_KICK=4,        //Message format "kicker kickee" FIXME
	
	CC_VOICE=5,
	CC_DEVOICE=6,
	CC_OP=7,
	CC_DEOP=8,
	CC_HOP=9,
	CC_DEHOP=10
};


//Class to receive all callbacks from an irc object
class ircCallback
{
public:
	//Messages from the server you may care about
	virtual void serverCallback       (const serverMessageType &type, const string &message) = 0;
	
	//Message in a channel from someone
	virtual void channelMsgFromCallback   (const string &channel, const string &who, const string &message) = 0;

	//Message in a channel from someone
	virtual void channelNoticeFromCallback   (const string &channel, const string &who, const string &message) = 0;

	//Message in a channel to someone
	virtual void channelMsgToCallback   (const string &channel, const string &who, const string &message) = 0;

	//Message in a channel to someone
	virtual void channelNoticeToCallback   (const string &channel, const string &message) = 0;
	
	//Channel mode changes
	//"root"  "+v jimmy"
	//These are sent before a channelChangeCallback for each of the actual changes
	virtual void channelModeCallback  (const string &channel, const string &whoDone,         const string &mode) = 0;
	
	//People join/leave chan
	virtual void channelChangeCallback(const string &channel, const channelChangeType &type, const string &who,  const string &message) = 0;

  //Topic
  virtual void channelTopicCallback(const string &channel, const string &topic) = 0;
  virtual void channelTopicChangedByCallback(const string &channel, const string &who, const string &stamp) = 0;
	
	//Private message from someone
	virtual void privateMsgFromCallback(const string &who, const string &message) = 0;

	//Private message to someone
	virtual void privateMsgToCallback(const string &who, const string &from, const string &message) = 0;
	
	//virtual ~ircCallback() = 0;
	irc* myirc; //should be static or put it else where as static
};

class ircPerson
{
public:
	ircPerson(); //DONT USE
	ircPerson(string joinInfo); // nick!user@host
	ircPerson(const string &nNick, const string &nRealname, const string &nUser, const string &nHost, const string &nMode);
	
//don't fuck with these
	bool voice;
	bool op;
	bool hop;

  string mode;
	string nick;
	string realname;
	bool knowrealname;
		
	string user;	// user@
	string host;	//     @mail.com
	            	// (@ character not included in either)
};

class channelDetails
{
public:
	channelDetails(); //<- DON'T USE, required so channelDetails can be used properly with a map
	channelDetails(const string &name, const string &topic);
	
	void addPerson(const ircPerson& person);
	void renamePerson(const string &oldNick, const string &newNick);
	
	//used by irc class when receiving a MODE message.
	//parameters will be of format: 
	//source - ":bob!aaa@bbb" (use getNickFromInfo) - person/server that did change
	//modeString - "+vv Randy DuEy" or "+t" etc...
	void changeMode(ircCallback* callback, const string source, const string modeString);

	void removePerson(const string &nick);
	
	string channelName;
	
	string channelTopic;
	
	string channelMode; //TODO?
	
	//List of all people in the channel
	list<ircPerson> channelPeople;
	//TODO: Maybe make this a map?
	
private:
	
	//Return a person from the channelPeople list, it is assumed the person exists.
	ircPerson& getPerson(const string& nick);
};


class irc
{
public:
	irc(ircCallback* CB);
	
	//Connect to an irc server, blocking
	//May generate multiple callbacks while running
	//if returns true then it is connected to a server and is running
	bool doConnect(const string &nServer, 
                 const int &nPort, 
                 const string &nUser, 
                 const string &nNick, 
                 const string &nRealname, 
                 const string &nPassword);
	
	//Disconnects from the server, nonblocking(?)
	void doDisconnect();
	
	//Poll the connection for waiting messages, blocking-ish
	//may generate callbacks if something has happened
	void poll();
	
	//send message to target (works for channels too), 
	void sendPM(const string &target, const string &message);

	//Simple way to send an action, works as above
	void sendAction(const string &target, const string &message);

	//send a raw command to the server (\r\n is automatically appended)
	void sendRaw(const string &message);
	void pingServer(); //non blocking, throws away output... not really a ping at all, lol
	
	//Away Modes
	void setAway(const string &message);
	void setBack();
	
	void whoIs(const string nick);
	
	void joinChannel(const string &channel);
	
	///Functions for getting information
	string getMyNick() const;
	
	const channelDetails& getChannelDetails(const string &channel) const;
	
	const map<string, channelDetails>& getChannels() const;
	
	//returns true if we are in the given channel
	bool inChannel(const string &channel) const;

  bool isRunning() const;
  void setRunning();
  void clearRunning();
  
  ircCallback* getCallback() const;
	
private:
	void sendData(const string &data);
	
	void processLineBuf();
	bool parseFirst();
	bool parseSecond();
	bool parseSecondString();
	
	//User Settings
	int    port;
	string mServer;
	string mUser;
	string mNickname;
	string mRealname;
  string mPassword;

	//Callbacks
	ircCallback* callback;
	
	//Connection
	int sock;
	
	connectionStatus status;
	
	string currentLineBuf;
	
	//Channels we're currently in with associated details
	map<string, channelDetails> channels;
};
# endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct irc_chantab_elem {
    char* name;
    char* topic;

  } irc_chantab_elem;

  typedef struct irc_chantab_list {
    int               number_chan;
    irc_chantab_elem* chan_array;

  } irc_chantab_list;

  typedef struct irc_user_elem {
    char* user;
    char* nick;
    char* realname;
    char* host;
    char* mode;

  } irc_user_elem;

  typedef struct irc_user_list {
    int            number_user;
    irc_user_elem* user_array;

  } irc_user_list;

/* A thread which implements the irc protocol */
extern int irc_thread_main(SceSize args, void *argp);

extern void irc_thread_join_channel(char* channel);
extern void irc_thread_send_message(char *channel, char *message);

extern int irc_thread_session_is_terminated();
extern void irc_thread_session_set_terminated();

extern irc_chantab_list* irc_thread_get_chantabs(); 
extern void irc_thread_free_chantabs(irc_chantab_list* chantab_list); 

extern irc_user_list* irc_thread_get_users(const char* channel_name); 
extern void irc_thread_free_users(irc_user_list* user_list); 
extern int irc_thread_run_script(char *FileName);

#ifdef __cplusplus
}
#endif

#endif
