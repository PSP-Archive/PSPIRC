/*
 *  Source code from afkim written by Danzel
 *
 *  Modified by Zx   (26/7/2007)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "irc_thread.h"
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <sys/select.h>
#include <SDL.h>

#include "psp_global.h"
#include "psp_login.h"
#include "psp_console.h"
#include "psp_irc.h"
#include "psp_sdl.h"

///Some internal Functions

//extract nick from a :bob!something@something string, if not valid return whole string
string getNickFromInfo(const string &info)
{
	int startPos = 0;
	if(info[0] == ':')
		startPos++;
	
	string::size_type exclamation = info.find("!", startPos);
	if (exclamation == string::npos)
		return info;
	else
		return info.substr(startPos, exclamation-startPos);
}

///ircPerson
ircPerson::ircPerson() {} //DONT USE
ircPerson::ircPerson(string joinInfo) // nick!user@host
{
	//set defaults
	voice = false;
	op = false;
	hop = false;
	realname = "";
	knowrealname = false;
	
	//Extract the details from the string
	if (joinInfo[0] == ':') joinInfo = joinInfo.substr(1);
	int exPos = joinInfo.find("!");
	nick = joinInfo.substr(0, exPos);
	int atPos = joinInfo.find("@", exPos);
	user = joinInfo.substr(exPos+1, atPos-exPos-1);
	host = joinInfo.substr(atPos+1, joinInfo.length());
}

ircPerson::ircPerson(const string &nNick, const string &nRealname, const string &nUser, const string &nHost, const string &nMode)
{
	nick = nNick;
	realname = nRealname;
	user     = nUser;
	host     = nHost;
/* 
   H - User is not /away (here)
   r - User is using a registered nickname
   B - User is a bot (+B)
   * - User is an IRC Operator
   ~ - User is a Channel Owner (+q)
   & - User is a Channel Admin (+a)
   @ - User is a Channel Operator (+o)
   % - User is a Halfop (+h)
   + - User is Voiced (+v)
   ! - User is +H and you are an IRC Operator
   ? - User is only visible because you are an IRC Operator
   -
*/
	if (nMode.find("+") != string::npos) voice = true; else voice = false;
	if (nMode.find("@") != string::npos) op    = true; else op    = false;
	if (nMode.find("%") != string::npos) hop   = true; else hop   = false;
  mode = nMode;
}

///channelDetails

//DON'T USE, required so channelDetails can be used properly with a map
channelDetails::channelDetails()
{
	channelName = "INVALIDCHANNEL";
}
 
channelDetails::channelDetails(const string &name, const string &topic)
{
	channelName = name;
	channelTopic = topic;
}

void channelDetails::addPerson(const ircPerson& person)
{
	//Insert in order
	list<ircPerson>::iterator iter;
	for (iter=channelPeople.begin(); iter != channelPeople.end(); iter++)
	{
		if ((*iter).nick > person.nick)
		{
			channelPeople.insert(iter, person);
			return;
		}
	}
	
	channelPeople.push_back(person);
}

void channelDetails::renamePerson(const string &oldNick, const string &newNick)
{
	list<ircPerson>::iterator iter;
	for (iter=channelPeople.begin(); iter != channelPeople.end(); iter++)
	{
		if ((*iter).nick == oldNick)
		{
			(*iter).nick = newNick;
			break;
		}
	}
}

void channelDetails::removePerson(const string& nick)
{
	list<ircPerson>::iterator iter;
	for (iter=channelPeople.begin(); iter != channelPeople.end(); iter++)
	{
		if ((*iter).nick == nick)
		{
			channelPeople.erase(iter);
			break;
		}
	}
}


//used by irc class when receiving a MODE message.
//parameters will be of format: 
//source - ":bob!aaa@bbb" (use getNickFromInfo) - person/server that did change
//modeString - "+vv Randy DuEy" or "+t" etc...
void channelDetails::changeMode(ircCallback* callback, const string source, const string modeString)
{
//TODO: doesn't handle many valid modes (topic?)

	vector < string > split = explode(modeString, ' ');
	int cur = 1; //index in split list currently applying to
	
	bool inPlus = true; //needs a default, who cares
	
	for (unsigned int a = 0; a < split[0].size(); a++)
	{
		switch(split[0][a])
		{
		case '+':
			inPlus = true;
			break;
		case '-':
			inPlus = false;
			break;
		
		case 'o':
			getPerson(split[cur]).op = inPlus;
			if (inPlus)	callback->channelChangeCallback(channelName, CC_OP, source, split[cur]);
			else		callback->channelChangeCallback(channelName, CC_DEOP, source, split[cur]);
			cur++;
			break;
			
		case 'v':
			getPerson(split[cur]).voice = inPlus;
			if (inPlus)	callback->channelChangeCallback(channelName, CC_VOICE, source, split[cur]);
			else		callback->channelChangeCallback(channelName, CC_DEVOICE, source, split[cur]);
			cur++;
			break;
			
		case 'h':
			getPerson(split[cur]).hop = inPlus;
			if (inPlus)	callback->channelChangeCallback(channelName, CC_HOP, source, split[cur]);
			else		callback->channelChangeCallback(channelName, CC_DEHOP, source, split[cur]);
			cur++;
			break;
		
		}
	}
}

ircPerson& channelDetails::getPerson(const string& nick)
{
	list<ircPerson>::iterator iter;
	for (iter = channelPeople.begin(); iter != channelPeople.end(); iter++)
	{
		if ((*iter).nick == nick)
			return *iter;
	}

	//THIS SHOULD NEVER HAPPEN
}

#if 0 //LUDO: def PSP
///PSP Specific resolver hack.... uses threads so that we don't get stuck if resolver locks up.
const char* PSP_strtoresolv = NULL;
struct hostent * PSP_resolvd = NULL;

int 
resolvThread(SceSize args, void *argp)
{
	PSP_resolvd = gethostbyname(PSP_strtoresolv);
	return 0;
}

struct hostent * PSP_gethostbyname(const char* addr, ircCallback* callback)
{
	PSP_strtoresolv = addr;
	PSP_resolvd = NULL;
	
	for (int a = 0; a < 4 && PSP_resolvd==NULL; a++) //try up to 4 times
	{
		SceUID dlthread = sceKernelCreateThread("pspirc_resolver", resolvThread, 0x18, 0x10000, 0, NULL);
		sceKernelStartThread(dlthread, 0, NULL);
		
		unsigned int startTime = sceKernelGetSystemTimeLow();
		do
		{
			sceKernelDelayThread(100*1000);
		} while (PSP_resolvd == NULL && startTime+2000*1000 >= sceKernelGetSystemTimeLow()); //not resolved, within 2 seconds
		
		if (PSP_resolvd == NULL)
		{
			callback->serverCallback(SM_IRC_DETAILS, "Timeout while resolving");
		}
		/* int ret = */ sceKernelTerminateDeleteThread(dlthread);
# if 0 //LUDO: DEBUG
		if (ret < 0) {
			cout << "Failed to kill downloading thread, ignoring. (this will likely cause problems)\n";
		}
# endif
	}
	
	return PSP_resolvd;
}
#endif

static bool volatile loc_running = false;

///irc
irc::irc(ircCallback* CB)
{
	callback = CB;
	CB->myirc = this;
	
	sock = 0;
  loc_running = true;
	status = CS_OFFLINE;
}

bool 
irc::doConnect(const string& nServer, 
               const int   & nPort, 
               const string& nUser, 
               const string& nNick, 
               const string& nRealname, 
               const string& nPassword)
{
	//Disconnect if connected.
	if (status != CS_OFFLINE)
		doDisconnect();
	
	mServer   = nServer;
	port      = nPort;
  mUser     = nUser;
	mNickname = nNick;
  mRealname = nRealname;
  mPassword = nPassword;
	
	status = CS_CONNECTING;
		
		//resolve
	struct hostent * resolvd;
	callback->serverCallback(SM_IRC_DETAILS, "Resolving");
#if 0 //LUDO: def PSP
	resolvd = PSP_gethostbyname(mServer.c_str(), callback);
#else
	resolvd = gethostbyname(mServer.c_str());
#endif
	if (resolvd == NULL)
	{
		status = CS_OFFLINE;
		callback->serverCallback(SM_IRC_ERROR, "Failed to resolve");
		return false;
	}
	callback->serverCallback(SM_IRC_DETAILS, "Resolved, Connecting");
	
		//attempt nonblocking connect
	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = ((struct in_addr *)(resolvd->h_addr))->s_addr;
	saddr.sin_port = htons(port);
	
	//Create socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		status = CS_OFFLINE;
		callback->serverCallback(SM_IRC_ERROR, "Socket Creation Error");
		return false;
	}
	
	//TODO make not blocking / add timeout
	if (connect(sock,(struct sockaddr *)  &saddr, sizeof(saddr)) == -1) {
		status = CS_OFFLINE;
		callback->serverCallback(SM_IRC_ERROR, "Connect error");
		return false;
	}
	
	status = CS_IDENTIFYING;
	callback->serverCallback(SM_IRC_DETAILS, "Connected, Identifying");
		
		//send login details (UNKN = client type)
  if (! mPassword.empty()) {
    string loginPass = "PASS " + mPassword + "\r\n";
	  sendData(loginPass);
  }
	string loginDetails = "NICK " + mNickname + "\r\nUSER " + mUser + " UNKN SERVERADDR :" + mRealname + "\r\n";
	sendData(loginDetails);
	
	//poll untill connected or failed.
	while(status == CS_CONNECTING || status == CS_IDENTIFYING)
	{
		poll();
		sceKernelDelayThread(1000*100);
	}
	
	if (status != CS_CONNECTED)
		return false;
	
	return true;
}

void irc::doDisconnect()
{
	sendData("QUIT :bai\r\n");
	close(sock);
	sock = 0;
	status = CS_OFFLINE;
	channels.clear();
}

const int TMP_BUF_SIZE = 1000;
char tmpBuf[TMP_BUF_SIZE];

void irc::poll()
{
	//Don't poll sockets if we are offline.
	if (status == CS_OFFLINE)
		return;
	
	fd_set socks;
	FD_ZERO(&socks);
	FD_SET(sock,&socks);
	
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;	

	int socksToRead = select(sock+1, &socks, (fd_set *) 0, (fd_set *) 0, &timeout);
	
	if (socksToRead < 0) {
		callback->serverCallback(SM_IRC_ERROR, "poll() Error");
    clearRunning();
		return;
	}
	else if (socksToRead == 0)
	{
		return;
	}
	
	//Socket has something
	int received = recv(sock, tmpBuf, TMP_BUF_SIZE-1, 0);
	if (received == -1)
	{
		close(sock);
		status = CS_OFFLINE;
		channels.clear();
		callback->serverCallback(SM_IRC_ERROR, "recv() Error");
    clearRunning();
		return;
	}
	
	//add all the new chars to the buffer, parsing it when we hit line ends
	for (int a = 0; a < received; a++)
	{
		if (tmpBuf[a] == '\r' || tmpBuf[a] == '\n')
		{
			processLineBuf();
		}
		else
		{
			currentLineBuf += tmpBuf[a];
		}
	}
  return;
}

void irc::sendPM(const string &target, const string &message)
{
	sendData("PRIVMSG " + target + " :" + message +"\r\n");
}

void irc::sendAction(const string &target, const string &message)
{
	sendData("PRIVMSG " + target + " :\1ACTION " + message +"\1\r\n");
}

void irc::setAway(const string &message)
{
	sendData("AWAY :" + message + "\r\n");
}

void irc::setBack()
{
	sendData("AWAY\r\n");
}

void irc::joinChannel(const string &channel)
{
	sendData("JOIN " + channel + "\r\n");
}

bool irc::isRunning() const
{
  return loc_running;
}

void irc::setRunning()
{
  loc_running = true;
}

void irc::clearRunning()
{
  loc_running = false;
}

ircCallback* 
irc::getCallback() const
{
  return callback;
}


//TODO: I SHOULD BE USING THIS EVERYWHERE, saves writing \r\n everywhere!
void irc::sendRaw(const string &message)
{
	sendData(message+"\r\n");
}

void irc::whoIs(const string nick)
{
	sendData("WHOIS " + nick + "\r\n");
}

void irc::pingServer()
{
	sendData("PING LAG123123123\r\n");
}

//PRIVATE
void irc::sendData(const string &data)
{
	if (send(sock, data.c_str(), data.length(), 0) == -1)
	{
		callback->serverCallback(SM_IRC_ERROR, "Error sending data");
	}
}

void irc::processLineBuf()
{
	if (currentLineBuf == "") return;
# if 0 //LUDO: DEBUG
	cout << "RAW: " << currentLineBuf << endl;
# endif
	//Try parse 
	if (!parseFirst())
	{
		if (!parseSecond())
		{
			if (!parseSecondString())
			{
				//Unable to parse, print raw
				callback->serverCallback(SM_IRC_UNK, currentLineBuf);
			}
		}
	}
	currentLineBuf = "";
}

//First and Seconds only contain that string
#define GET_FIRST();	string::size_type firstSpacePos = currentLineBuf.find(" "); \
						if (firstSpacePos == string::npos) return false; \
						string first = currentLineBuf.substr(0, firstSpacePos);

#define GET_SECOND();	string::size_type secondSpacePos = currentLineBuf.find(" ", firstSpacePos+1); \
						if (secondSpacePos == string::npos) return false; \
						string second = currentLineBuf.substr(firstSpacePos+1, secondSpacePos-firstSpacePos-1);

#define GET_THIRD(); 	string::size_type thirdSpacePos = currentLineBuf.find(" ", secondSpacePos+1); \
						if (thirdSpacePos == string::npos) return false; \
						string third = currentLineBuf.substr(secondSpacePos+1, thirdSpacePos-secondSpacePos-1); \
						if (third[0] == ':') third = third.substr(1, third.length());

#define GET_FOURTH(); 	string::size_type fourthSpacePos = currentLineBuf.find(" ", thirdSpacePos+1); \
						if (fourthSpacePos == string::npos) return false; \
						string fourth = currentLineBuf.substr(thirdSpacePos+1, fourthSpacePos-thirdSpacePos-1); \
						if (fourth[0] == ':') fourth = fourth.substr(1, fourth.length());

#define GET_FIFTH(); 	string::size_type fifthSpacePos = currentLineBuf.find(" ", fourthSpacePos+1); \
						if (fifthSpacePos == string::npos) return false; \
						string fifth = currentLineBuf.substr(fourthSpacePos+1, fifthSpacePos-fourthSpacePos-1); \
						if (fifth[0] == ':') fifth = fifth.substr(1, fifth.length());

//Third and Fourth plus contain all strings after that space (fourth+ requres GET_THIRD called first)
#define GET_THIRD_PLUS();	string third = currentLineBuf.substr(secondSpacePos+1, currentLineBuf.length()); \
							if(third[0] == ':') third=third.substr(1, third.length());

#define GET_FOURTH_PLUS();	string fourth = currentLineBuf.substr(thirdSpacePos+1, currentLineBuf.length()); \
							if(fourth[0] == ':') fourth=fourth.substr(1, fourth.length());

#define GET_SIXTH_PLUS(); string sixth = currentLineBuf.substr(fifthSpacePos+1, currentLineBuf.length()); \
							if(sixth[0] == ':') sixth=sixth.substr(1, sixth.length());


bool irc::parseFirst()
{
	GET_FIRST();
	
	if (first == "PING") //PING
	{
		currentLineBuf[1] = 'O';
		sendData(currentLineBuf+"\r\n");
#ifdef DEBUG
		callback->serverCallback(SM_GENERAL_TEXT, "Ping? Pong!");
#endif
		return true;
	}
	
	if (first == "NOTICE") //NOTICE : this server is liek leet pwnz0r!
	{
		callback->serverCallback(SM_GENERAL_TEXT, currentLineBuf);
		return true;
	}
	
	return false;
}

bool irc::parseSecond()
{
	GET_FIRST();
	GET_SECOND();
	
	int secNum = atoi(second.c_str());
	if (secNum != 0) //It is a number
	{
		switch(secNum)
		{
		//Things we dont really care about
		
		///Things we chose to ignore completely
		case 353: //people in a /names list
		case 366: //end of /names list
		case 221: // user mode is... (:scraps.workgroup 221 danzel +s)
			return true;
		///Straight message from the server to pass on.
		case 1:
		case 2:
		case 3:
		case 4://ServerName ServerVersion something something something
		case 5://Settings such supported by the server (possibly useful?)
			if (status == CS_IDENTIFYING)//Any of these mean that we are now connnected
			{
				status = CS_CONNECTED;   //We are connected yay!
				return true;
			}
		///Things that dont really matter, but are server messages
		case 251://User Invisible Servers
		case 252://ops online
		case 253://unknown connections
		case 254://channels formed
		case 255://i have X clients and Y servers (local server)
		case 375://pre MOTD (Start of MOTD?)
		case 372://do XXXX to read the MOTD
		case 376://end of MOTD
		{
			GET_THIRD();
			GET_FOURTH_PLUS();
			callback->serverCallback(SM_GENERAL_TEXT, fourth);
			return true;
		}

    //LUDO: Channel List
    case 321: //Channel Users Topic
    {
			GET_THIRD();
			GET_FOURTH_PLUS();
			callback->serverCallback(SM_LIST_BEGIN, fourth);
			return true;
    }

    case 322: //New item in the channel list
    {
			GET_THIRD();
			GET_FOURTH_PLUS();
			callback->serverCallback(SM_LIST_CHAN, fourth);
			return true;
    }
    case 323: //End of channel list
    {
			GET_THIRD();
			GET_FOURTH_PLUS();
			callback->serverCallback(SM_LIST_END, fourth);
			return true;
		}
		
		///Error messages
		case 404://cannot send to channel
		case 421://unknown command
		{
			GET_THIRD_PLUS();
			callback->serverCallback(SM_NONFATAL_ERROR, third);
			return true;
		}
		case 433: //Nickname in use
		{
			GET_THIRD();
			GET_FOURTH();
			callback->serverCallback(SM_NICK_TAKEN, fourth);
			return true;
		}
		
		///Things we need to do something about
		
		case 332: //Channel topic is...
		{
			//:Oslo2.NO.EU.undernet.org 332 ircbotest #hamlan :hamLan V | 20-21 Jan 2007 | http://register.hamlan.co.nz/ | http://imdb.com/title/tt0479884/ you will watch
			//:server 332 whodidit channel topicishere
			//Get the chan
			GET_THIRD();
			GET_FOURTH();
			
			//FIXME: GET_FIFTHPLUS
			string topic;
			if (currentLineBuf[fourthSpacePos+1] == ':')
				topic = currentLineBuf.substr(fourthSpacePos+2, currentLineBuf.length());
			else
				topic = currentLineBuf.substr(fourthSpacePos+1, currentLineBuf.length());
			
			if (inChannel(fourth))
			{
				channels[fourth].channelTopic = topic;
			}
			
			callback->channelTopicCallback(fourth, topic);
			return true;
		}
		case 333: //Who Set the topic
    {
			GET_THIRD();
			GET_FOURTH();
      GET_FIFTH();
      GET_SIXTH_PLUS();

      time_t my_time = atol(sixth.c_str());
	    std::string my_time_str( ctime(&my_time) );
      my_time_str.resize(my_time_str.length()-1);
			callback->channelTopicChangedByCallback(fourth, fifth, my_time_str);

      return true;
    }
		case 352: //People in ... is ..... (WHO list)
		{
			//"<server> 352 YOURNICK <channel> <user> <host> <server> <nick> <H|G>[*][@|+] :<hopcount> <real name>"
			// :im.bitlbee.org 352 danzel &bitlbee danzel 222-152-204-160.jetstream.xtra.co.nz im.bitlbee.org danzel H :0 Unknown
			// :im.bitlbee.org 352 danzel &bitlbee root im.bitlbee.org im.bitlbee.org root H :0 User manager
			
			//  0              1   2       3         4        5         6              7      8              9          10
			// <server>        352 URNICK <channel> <user>   <host>    <server>       <nick> <H|G>[*][@|+] :<hopcount> <real name>
			// :im.bitlbee.org 352 danzel &bitlbee stigmannz sdm.co.nz im.bitlbee.org stigmannz G :0 Rick Blaine ...
			// :im.bitlbee.org 315 danzel &bitlbee :End of /WHO list.
			
			vector<string> exploded = explode(currentLineBuf, ' ');
			
			if (!inChannel(exploded[3])) //someone from a channel we aren't in, WTF?
				return true;
			
			//check that this person isnt already in the chan.
			channels[exploded[3]].removePerson(exploded[7]);
			channels[exploded[3]].addPerson(ircPerson(exploded[7], exploded[10], exploded[4], exploded[5], exploded[8]));

      //LUDO:
			callback->serverCallback(SM_WHO, 
            exploded[3]  + " "
          + exploded[7]  + " "
          + exploded[10] + " "
          + exploded[4]  + " " 
          + exploded[5]  + " "
          + exploded[8] );

			return true;
		}
		case 315: //End of /who list
		{
			// :ny.undernet.org 315 YOURNICK #CHANNEL :End of /WHO list.
			GET_THIRD();
			GET_FOURTH();
			
			callback->serverCallback(SM_WHO_DONE, fourth);
			
			return true;
		}

		case 367: //LUDO: Ban list entry
    {
			GET_THIRD();
			GET_FOURTH();
      GET_FIFTH();
      GET_SIXTH_PLUS();
      time_t my_time = atol(sixth.c_str());
	    std::string my_time_str( ctime(&my_time) );
      my_time_str.resize(my_time_str.length()-1);
			callback->serverCallback(SM_BANLIST, fourth + " " + fifth + " " + my_time_str );

			return true;
    }

    case 368: // Endof 
    {
			// :ny.undernet.org 368 YOURNICK #CHANNEL :End of channel Ban list.
			GET_THIRD();
			GET_FOURTH();
			
			callback->serverCallback(SM_BANLIST_DONE, fourth);
			return true;
    }
		
		//A couple of whois responses
		case 311: //Users realname and ipaddress
		{
			//:scraps 311 danzel thorxxx thorxxx hotmail.com * :Damion
			GET_THIRD();
			GET_FOURTH();
			
			//FIXME: finding ':' is a bit naughty
			string realname = currentLineBuf.substr(currentLineBuf.find(":", thirdSpacePos)+1, currentLineBuf.length());
			
			//FIXME: Should also get their IP and send it tooo~
			callback->serverCallback(SM_WHOIS_REALNAME, fourth + " " + realname);
			
			return true;
		}
		case 312: //server the user is connected to
		{
			//:scraps 312 danzel thorxxx my@msn_email.com. :MSN network
			
			GET_THIRD();
			GET_FOURTH();
			
			//FIXME: finding ':' is a bit naughty
			string theirserver = currentLineBuf.substr(currentLineBuf.find(":", thirdSpacePos)+1, currentLineBuf.length());
			
			callback->serverCallback(SM_WHOIS_SERVER, fourth + " " + theirserver);
			return true;
		}
		case 301: //Away info
		{
			//:scraps 301 danzel thor98x :Away
			GET_THIRD();
			GET_FOURTH();
			
			//FIXME: GET_FIFTHPLUS
			string awaytxt;
			if (currentLineBuf[fourthSpacePos+1] == ':')
				awaytxt = currentLineBuf.substr(fourthSpacePos+2, currentLineBuf.length());
			else
				awaytxt = currentLineBuf.substr(fourthSpacePos+1, currentLineBuf.length());
			
			callback->serverCallback(SM_WHOIS_AWAY, fourth + " " + awaytxt);
			return true;
		}
		case 318: //End of whois
		{
			//:scraps 318 danzel thor98x :End of /WHOIS list
			GET_THIRD();
			GET_FOURTH();
			
			callback->serverCallback(SM_WHOIS_END, fourth);
			
			return true;
		}

		} //switch
	}
	
	return false;
}

bool irc::parseSecondString()
{
	GET_FIRST();
	GET_SECOND();
	
  //:<FROM_NICK>!<USERNAME>@<HOST> NOTICE <TO_NICK>: <MSG>
	if (second == "NOTICE") //Notice Message
	{
		GET_THIRD();
    GET_FOURTH_PLUS();
		string nick = getNickFromInfo(first);
    const char* a_chan = psp_console_get_current_channel();
    if (nick[0] == ':') {
		  callback->serverCallback(SM_NOTICE, fourth);
    } else {
      if (a_chan) {
        std::string cur_chan(a_chan);
			  callback->channelNoticeFromCallback(cur_chan, "-"+nick+"/"+third+"-", fourth);
      } else {
			  callback->serverCallback(SM_NOTICE, "-"+nick+"/"+third+"-: "+fourth);
      }
    }
		return true;
	}
	else if (second == "NICK")// nick change message
	{
		// :<NICK>!<USERNAME>@<HOST> NICK <NEWNICK>
		GET_THIRD_PLUS(); //third = newnick
		string oldnick = getNickFromInfo(first);
		
		if (oldnick == mNickname) mNickname = third; //If its our nickname

		//Rename the person in the nick list
		map<string,channelDetails>::iterator iter;
		for (iter = channels.begin(); iter != channels.end(); iter++)
		{
			iter->second.renamePerson(oldnick, third);
		}
		callback->serverCallback(SM_RENAME, oldnick+" to "+third);
		
		return true;
	}
	else if (second == "PART") //Part message
	{
		//:dapples!dsawerwaer PART :#hamlan
		
		GET_THIRD_PLUS(); //HACK
		
		//TODO: Remove this hack and use get_third get_fourth+, 3=chan, 4=reason (may not be one)
		string::size_type spacePos;
		if ((spacePos = third.find(' ')) != string::npos)
		{
			third = third.substr(0,spacePos);
		}
		
		if (!inChannel(third)) //we aren't in this channel, wtf?
			return true;
		
		//get usernick
		string partnick = getNickFromInfo(first);

		channels[third].removePerson(partnick);
		
		callback->channelChangeCallback(third, CC_PART, partnick, partnick);
		
		if (partnick == mNickname) //if we've left
		{
			channels.erase(channels.find(third)); //leave the channel
			callback->serverCallback(SM_LEFT_CHAN, third);
		}
		return true;
	}
	else if (second == "JOIN") //Join message
	{
		GET_THIRD_PLUS();
		
		if (third[0] == ':') third = third.substr(1, third.length());
		string joinnick = getNickFromInfo(first);
		
		//its us, get list of people in chan
		if (joinnick == mNickname) 
		{
			if (!inChannel(third)) {
				channels[third] = channelDetails(third, "no topic");
      }
			channels[third].addPerson(ircPerson(first)); //Add ourself
			
			sendData("WHO " + third + "\r\n");
			callback->serverCallback(SM_JOIN_CHAN, third);
			return true;
		}
		
		if (!inChannel(third)) //channel we don't care about
			return true;
		
		channels[third].removePerson(joinnick);
		channels[third].addPerson(ircPerson(first));
		callback->channelChangeCallback(third, CC_JOIN, joinnick, joinnick);
		return true;
	}
	else if (second == "KICK") //Kick message
	{
		//:WiZ KICK #Finnish John :Speaking English
		
		GET_THIRD();
		GET_FOURTH_PLUS();//HACK as there may not be a reason.
		string::size_type spacePos;
		if ((spacePos = fourth.find(" ")) != string::npos)
			fourth = fourth.substr(0, spacePos);
		
		callback->channelChangeCallback(third, CC_KICK, first,  fourth); //FIXME
		
		if (fourth == mNickname) //if we've left
		{
			channels.erase(channels.find(third)); //leave the channel
			callback->serverCallback(SM_LEFT_CHAN, third);
		}
		
		return true;
	}
	else if (second == "QUIT") //Quit message
	{
		GET_THIRD_PLUS();
		
		//get usernick
		string quitnick = getNickFromInfo(first);

		//Rename the person in the nick list
		map<string,channelDetails>::iterator iter;
		for (iter = channels.begin(); iter != channels.end(); iter++)
		{
			iter->second.removePerson(quitnick);
		}
		
		callback->serverCallback(SM_QUIT, quitnick + " has quit " + third);
		return true;
	}
	else if (second == "MODE") //Mode message
	{
		//TODO: "server sets mode +x danzel" ??
	
		//:danzel!aaa@bbb MODE #hamlan +vv Randy DuEy
		GET_THIRD();
		GET_FOURTH_PLUS();
		
		string modenick = getNickFromInfo(first);
		
		
		if (!inChannel(third))
			return true;
		
		channels[third].changeMode(callback, modenick, fourth);
		
		callback->channelModeCallback(third, modenick, currentLineBuf.substr(secondSpacePos+1));
		return true;
	}
	else if (second == "TOPIC") //Topic Change message
	{
		//:danzel!fff@fff TOPIC #hamlan :safisjdf saeaower sadflksdf
		GET_THIRD();
		GET_FOURTH_PLUS();
		
		string topicnick = getNickFromInfo(first);
		
		if (!inChannel(third)) //Topic for a channel I aren't in? wtf?
			return true;
		
		channels[third].channelTopic = fourth; //TODO: Function?
		
		callback->channelChangeCallback(third, CC_TOPIC, topicnick, fourth);
		return true;
	}
	else if (second == "INVITE") //Invite message
	{
		GET_THIRD();
		GET_FOURTH_PLUS();
		
		//:Angel!wings@irc.org INVITE Wiz #Dust
		string invitenick = getNickFromInfo(first);
		
		callback->serverCallback(SM_INVITE, invitenick + " to " + fourth);
		
		return true;
	}
	else if (second == "PRIVMSG") //Private message
	{
		GET_THIRD();
		GET_FOURTH_PLUS();
		//privateMsgCallback   (string who,             string message)
		//channelMsgCallback   (string who,             string message)
		
		//:DuEy!aaa@aaa PRIVMSG #hamlan :omfg wtf bix nood
		//:danzel!~danzel@Danzel.users.undernet.org PRIVMSG #hamlan :\001ACTION tests\001
		//:danzel!~danzel@Danzel.users.undernet.org PRIVMSG #hamlan :\001VERSION\001
		//^^ reply  NOTICE danzel :\001VERSION STRING\001
		//TODO VERSION/ACTION
		string sender = getNickFromInfo(first);
		
		if (third == mNickname)	//PM
		{
			callback->privateMsgFromCallback(sender, fourth);
		}
		else	//Chan
		{
			callback->channelMsgFromCallback(third, sender, fourth);
		}
		return true;
	}
	else if (second == "PONG") //pong reply, throw it away
	{
		return true;
	}
	
	return false;
}

///Functions for getting information
string irc::getMyNick() const
{
	return mNickname;
}

const map<string, channelDetails>& irc::getChannels() const
{
	return channels;
}

//This won't compile as channels[channel] isn't really const :(
//TODO: do this a smarter way
const channelDetails& irc::getChannelDetails(const string &channel) const
{
	map<string, channelDetails>::const_iterator iter;
	for (iter = channels.begin(); iter != channels.end(); iter++)
	{
		if (iter->first == channel)
		{
			return iter->second;
		}
	}
	
	//OH SHI- NOT ALL RETURN ONE OHES NOES
}

bool irc::inChannel(const string &channel) const
{
	return channels.count(channel) != 0;
}

class myircCallback : public ircCallback {

	virtual void serverCallback(const serverMessageType &type, const string &message)
  {
    bool display = true;
    int color = COLOR_GREEN;
    std::string display_msg(message);

    switch (type) {
	    case SM_IRC_UNK : 
      break;
	    case SM_IRC_DETAILS : 
      break;
	    case SM_IRC_ERROR : 
      break;
	    case SM_GENERAL_TEXT : 
      break;
	    case SM_NOTICE: 
      break;
	    case SM_RENAME : 
        {
          color = COLOR_BLUE;
          display_msg = "rename " + message;
        }
      break;
	    case SM_NONFATAL_ERROR :
      break;
	    case SM_NICK_TAKEN : 
        {
          color = COLOR_RED;
          display_msg = "nickname " + message + " already in use";
        }
      break;
	    case SM_QUIT : 
      break;
	    case SM_INVITE : 
        {
          color = COLOR_RED;
          display_msg = "you have been invited by " + message;
        }
      break;
	    case SM_JOIN_CHAN : psp_console_add_tab(message.c_str(), CONSOLE_TYPE_CHANNEL);
                          display = false;
      break;
	    case SM_LEFT_CHAN : psp_console_del_tab(message.c_str());
                          display = false;
      break;
      case SM_WHO :
        {
          display_msg = "who: " + message;
          color = COLOR_BLUE;
        }
      break;
	    case SM_WHO_DONE : display = false;
      break;
	    case SM_UNKNOWN : 
      break;
      case SM_BANLIST :
        {
          display_msg = "banlist: " + message;
          color = COLOR_BLUE;
        }
      break;
      case SM_BANLIST_DONE : display = false;
      break;
	
	    case SM_WHOIS_IP : 
	    case SM_WHOIS_REALNAME : 
	    case SM_WHOIS_SERVER : 
	    case SM_WHOIS_AWAY : 
        {
          display_msg = "whois: " + message;
          color = COLOR_BLUE;
        }
      break;
	    case SM_WHOIS_END : display = false;
      break;
      case SM_LIST_BEGIN : if (psp_irc_chanlist_is_running()) {
                             psp_irc_chanlist_begin();
                             display = false;
                           }
                           color = COLOR_BLUE;
      break;
      case SM_LIST_CHAN :  if (psp_irc_chanlist_is_running()) {
                             psp_irc_add_chanlist(message.c_str()); 
                             display = false;
                           }
                           display_msg = "list: " + message;
                           color = COLOR_BLUE;
      break;
      case SM_LIST_END  :  if (psp_irc_chanlist_is_running()) { 
                             psp_irc_chanlist_end();
                           }
                           display = false;
      break;
    }

    if (display) {
      std::ostringstream stm;
#ifdef DEBUG
      stm << "[" << type << "] " << display_msg << std::endl;
# else
      stm << display_msg << std::endl;
#endif
      psp_console_write_console((char *)stm.str().c_str(), color);
    }
  }
	
	//Message in a channel from someone
	virtual void channelMsgFromCallback(const string &channel, const string &who, const string &message)
  {
    std::ostringstream stm;
    stm << who << ": " << message << std::endl;
    psp_console_write_channel(channel.c_str(), stm.str().c_str(), COLOR_OTHER);
  }

	//Message in a channel from someone
	virtual void channelNoticeFromCallback(const string &channel, const string &who, const string &message)
  {
    std::ostringstream stm;
    stm << who << ": " << message << std::endl;
    psp_console_write_channel(channel.c_str(), stm.str().c_str(), COLOR_BROWN);
  }

	//Message in a channel from someone
	virtual void channelMsgToCallback(const string &channel, const string &who, const string &message)
  {
    std::ostringstream stm;
    stm << who << ": " << message << std::endl;
    psp_console_write_channel(channel.c_str(), stm.str().c_str(), COLOR_USER);
  }

	//Message in a channel from someone
	virtual void channelNoticeToCallback(const string &channel, const string &message)
  {
    std::ostringstream stm;
    stm << " [notice] " << message << std::endl;
    psp_console_write_channel(channel.c_str(), stm.str().c_str(), COLOR_PINK);
  }
	
	//Channel mode changes
	//"root"  "+v jimmy"
	//These are sent before a channelChangeCallback for each of the actual changes
	virtual void channelModeCallback(const string &channel, const string &whoDone, const string &mode)
  {
    std::ostringstream stm;
    stm << whoDone << ": " << mode << std::endl;
    psp_console_write_channel(channel.c_str(), stm.str().c_str(), COLOR_BLUE);
  }
	
	//People join/leave chan
	virtual void channelChangeCallback(
       const string            &channel, 
       const channelChangeType &type, 
       const string            &who,
       const string            &message)
  {
    std::ostringstream stm;
    switch( type ) {
	    case CC_TOPIC:   stm << who << " has changed the topic to " << message << std::endl;
      break;           
	    case CC_JOIN:    stm << who << " has joined " << channel << std::endl;
      break;           
	    case CC_PART:    stm << who << " has left " << channel << std::endl;
      break;           
	    case CC_KICK:    stm << who << " has kicked " << message << " from " << channel << std::endl;
      break;           
	    case CC_VOICE:   stm << who << " gives voice to " << message << std::endl;
      break;
	    case CC_DEVOICE: stm << who << " removes voice from " << message << std::endl;
      break;
	    case CC_OP:      stm << who << " gives channel operator status to " << message << std::endl;
      break;
	    case CC_DEOP:    stm << who << " removes channel operator status from " << message << std::endl;
      break;
	    case CC_HOP:     stm << who << " gives channel half-operator status to " << message << std::endl;
      break;
	    case CC_DEHOP:   stm << who << " removes channel half-operator status from " << message << std::endl;
      break;
      default:  /* should not happen */
#ifdef DEBUG
       stm << type << ": " << message << std::endl;
# else
       stm << message << std::endl;
# endif
      break;
    }
    psp_console_write_channel(channel.c_str(), stm.str().c_str(), COLOR_YELLOW);
  }

  virtual void channelTopicCallback(const string &channel, const string &topic)
  {
    std::ostringstream stm;
    stm << "topic for " << channel << " is: " << topic << std::endl;
    psp_console_write_channel(channel.c_str(), stm.str().c_str(), COLOR_YELLOW);
  }

  virtual void channelTopicChangedByCallback(const string &channel, const string &who, const string &stamp)
  {
    std::ostringstream stm;
    stm << "topic for " << channel << " set by " << who << " at " << stamp << std::endl;
    psp_console_write_channel(channel.c_str(), stm.str().c_str(), COLOR_YELLOW);
  }
	
	//Private message from someone
	virtual void privateMsgFromCallback(const string &who, const string &message)
  {
    std::ostringstream stm;
    stm << who << ": " << message << std::endl;
    psp_console_write_private(who.c_str(), stm.str().c_str(), COLOR_OTHER);
  }

	//Private message to someone
	virtual void privateMsgToCallback(const string &who, const string &from, const string &message)
  {
    std::ostringstream stm;
    stm << from << ": " << message << std::endl;
    psp_console_write_private(who.c_str(), stm.str().c_str(), COLOR_USER);
  }
   
};

extern "C" {

static irc* volatile loc_irc_session = NULL;

void
irc_thread_join_channel(char* channel)
{
  std::string my_channel(channel);
  loc_irc_session->joinChannel(my_channel);
}

void
irc_thread_send_command(char *message)
{
  if (message[0] != '/') return;
  message++;

  std::string my_message(message);

  /* Specific treatment for /TOPIC */
  if (! strncasecmp(message, "TOPIC ", 6)) {
    char* scan_channel = message + 6;
    if (scan_channel[0] != '#') {
      char* current_channel = psp_console_get_current_channel();
      if (current_channel) {
        my_message.insert(scan_channel - message, 1, ' ');
        my_message.insert(scan_channel - message, current_channel);
      }
      message = (char *)my_message.c_str();
      scan_channel = message + 6;
    }
    char* scan_title = strchr(scan_channel + 1, ' ');
    if (scan_title) {
      scan_title++;
      my_message.insert(scan_title - message, 1, ':');
    }
  } else 
  /* Specific treatment for /NOTICE */
  if (! strncasecmp(message, "NOTICE ", 7)) {
    char* current_channel = psp_console_get_current_channel();
    if (current_channel) {
      std::string my_channel(current_channel);
      my_message = std::string(message + 7);
      loc_irc_session->getCallback()->channelNoticeToCallback(my_channel, my_message);
    }
    
  } else 
  /* Specific treatment for /MSG */
  if (! strncasecmp(message, "MSG", 3)) {
    my_message.insert(0, "PRIV");
  } else
  /* Specific treatment for /KICK */
  if (! strncasecmp(message, "KICK ", 5)) {
    char* scan_channel = message + 5;
    if (scan_channel[0] != '#') {
      char* current_channel = psp_console_get_current_channel();
      if (current_channel) {
        my_message.insert(scan_channel - message, 1, ' ');
        my_message.insert(scan_channel - message, current_channel);
      }
    }
  } else
  /* Specific treatment for /MODE */
  if (! strncasecmp(message, "MODE ", 5)) {
    char* scan_channel = message + 5;
    if (scan_channel[0] != '#') {
      char* current_channel = psp_console_get_current_channel();
      if (current_channel) {
        my_message.insert(scan_channel - message, 1, ' ');
        my_message.insert(scan_channel - message, current_channel);
      }
    }
  }
  loc_irc_session->sendRaw(my_message);
}


void
irc_thread_send_message(char *channel, char *message)
{
  std::string my_channel(channel);
  if (message[0] == '/') {
    irc_thread_send_command(message);
  } else {
    std::string my_message(message);
    if (! my_channel.empty()) {
      loc_irc_session->sendPM(my_channel, my_message);
      loc_irc_session->getCallback()->channelMsgToCallback(my_channel, loc_irc_session->getMyNick(), my_message);
    } else {
      psp_console_write_current("No channel joined. Try /join #<channel>\r\n", COLOR_RED);
    }
  }
}

void
irc_thread_send_private(char *who, char *message)
{
  std::string my_who(who);
  if (message[0] == '/') {
    std::string my_message(message + 1);
    loc_irc_session->sendRaw(my_message);
  } else {
    std::string my_message(message);
    if (! my_who.empty()) {
      loc_irc_session->sendPM(my_who, my_message);
      loc_irc_session->getCallback()->privateMsgToCallback(my_who, loc_irc_session->getMyNick(), my_message);
    } else {
      psp_console_write_current("No channel joined. Try /join #<channel>\r\n", COLOR_RED);
    }
  }
}

int
irc_thread_session_is_terminated()
{
  return ! loc_running;
}

void
irc_thread_session_set_terminated()
{
  loc_running = false;
}

static SceUID protocolthid = -1;

void
irc_thread_start_proto_thread()
{
  //Launch the protocol thread for that favorite
  protocolthid = sceKernelCreateThread("Protocol Thread", irc_thread_main , 0x18, 128*1024, 
                  PSP_THREAD_ATTR_USER, 0);
  if (protocolthid >= 0) {
    sceKernelStartThread(protocolthid, 0, NULL);
  } else {
    psp_sdl_exit(1);
  }
}

void
irc_thread_stop_proto_thread()
{
  irc_thread_session_set_terminated();
  sceKernelDelayThread(1000000);
  protocolthid = -1;
}

static void
loc_build_channel_list( std::list< string >& my_channel_list )
{
  char buffer[256];
  strcpy(buffer, psp_login_current->login.channel);
  char* beg_str = buffer;
  char* end_str = buffer;
  do {
    beg_str[0] = '#';
    end_str = strchr(beg_str + 1, '#');
    if (end_str) {
      *end_str = 0;
    }
    my_channel_list.push_back(std::string(beg_str));
    beg_str = end_str;

  } while (beg_str);

  if (my_channel_list.empty()) {
    my_channel_list.push_back("#psp");
  }
}

int
irc_thread_run_script(char *FileName)
{
  char  Buffer[512];
  FILE* FileDesc;
  char *Scan;

  FileDesc = fopen(FileName, "r");
  if (FileDesc == (FILE *)0 ) return -1;

  while (fgets(Buffer,512, FileDesc) != (char *)0) {

    Scan = strchr(Buffer,'\n');
    if (Scan) *Scan = '\0';
    /* For this #@$% of windows ! */
    Scan = strchr(Buffer,'\r');
    if (Scan) *Scan = '\0';
    if (Buffer[0] == '#') continue;

    std::string my_message(Buffer);
    if (! my_message.empty()) {
      loc_irc_session->sendRaw(my_message);
    }
  }
  fclose(FileDesc);
  return 0;
}

static int
loc_run_startup_script()
{
  char  FileName[MAX_PATH];
  snprintf(FileName, MAX_PATH, "%s/run/%s", PSPIRC.psp_homedir, psp_login_current->login.script );
  return irc_thread_run_script(FileName);
}

int 
irc_thread_main(SceSize args, void *argp)
{
  myircCallback my_irc_callback;
  loc_irc_session  = new irc(&my_irc_callback);

  std::string my_server_name = psp_login_current->login.host;
  std::string my_nickname = psp_login_current->login.nickname;
  std::string my_realname = psp_login_current->login.realname;
  std::string my_user = psp_login_current->login.user;
  std::string my_password = psp_login_current->login.password;
  std::string my_script = psp_login_current->login.script;
  int my_port = atoi(psp_login_current->login.port);

  std::list<string> my_channel_list;
  loc_build_channel_list(my_channel_list);

  if (loc_irc_session->doConnect(my_server_name, my_port, my_user, my_nickname, my_realname, my_password)) {

    std::string my_channel_name(my_channel_list.front());
    loc_irc_session->joinChannel(my_channel_name);

	  while (!loc_irc_session->inChannel(my_channel_name)) {
      if (!loc_irc_session->isRunning()) break;
		  loc_irc_session->poll();
		  sceKernelDelayThread(100);
    }

    /* Join to all channels but skip first one */
    std::list< string >::iterator ii = my_channel_list.begin();
    ++ii;
    while (ii != my_channel_list.end()) {
      loc_irc_session->joinChannel((*ii));
      ++ii;
    }

    /* If startup script has been specified then it is time to run it ! */
    if (! my_script.empty()) {
      loc_run_startup_script();
    }

    while (loc_irc_session->isRunning()) {
      loc_irc_session->poll();
		  sceKernelDelayThread(100);
    }

    loc_irc_session->doDisconnect();
  }

  loc_irc_session->clearRunning();

  delete loc_irc_session;
  loc_irc_session = 0;

  return 0;
}

irc_chantab_list*
irc_thread_get_chantabs()
{
  irc_chantab_list* chantab_list = (irc_chantab_list* )malloc( sizeof(irc_chantab_list) );
  memset(chantab_list, 0, sizeof(irc_chantab_list));

  const map<string, channelDetails>& my_channels = loc_irc_session->getChannels();
  int number_chan = my_channels.size();
  chantab_list->number_chan = number_chan;

  if (number_chan) {
    chantab_list->chan_array = (irc_chantab_elem* )malloc( sizeof( irc_chantab_elem ) * number_chan );
    int index = 0;
    for (map<string, channelDetails>::const_iterator ii = my_channels.begin(); ii != my_channels.end(); ++ii) {
      chantab_list->chan_array[index].name = strdup( (*ii).second.channelName.c_str() );
      chantab_list->chan_array[index].topic = strdup( (*ii).second.channelTopic.c_str() );
      index++;
    }
  }
  return chantab_list;
}

void
irc_thread_free_chantabs(irc_chantab_list* chantab_list)
{
  for (int index = 0; index < chantab_list->number_chan; ++index) {
    free(chantab_list->chan_array[index].name);
    free(chantab_list->chan_array[index].topic);
  }
  if (chantab_list->chan_array) {
    free(chantab_list->chan_array);
  }
  free(chantab_list);
}

irc_user_list*
irc_thread_get_users(const char *channel_name)
{
  const map<string, channelDetails>& my_channels = loc_irc_session->getChannels();
  std::string my_channel_name(channel_name);

  irc_user_list* user_list = (irc_user_list* )malloc( sizeof(irc_user_list) );
  memset(user_list, 0, sizeof(irc_user_list));

  if (my_channels.find(my_channel_name) == my_channels.end()) {
    return user_list;
  }

	const channelDetails my_channel_details = loc_irc_session->getChannelDetails(my_channel_name);
  const list<ircPerson>& my_users = my_channel_details.channelPeople;
  int number_user = my_users.size();
  user_list->number_user = number_user;

  if (number_user) {
    user_list->user_array = (irc_user_elem* )malloc( sizeof( irc_user_elem ) * number_user );
    int index = 0;
    for (list<ircPerson>::const_iterator ii = my_users.begin(); ii != my_users.end(); ++ii) {
      const ircPerson & my_person = (*ii);
      user_list->user_array[index].user = strdup( my_person.user.c_str() );
      user_list->user_array[index].nick = strdup( my_person.nick.c_str() );
      user_list->user_array[index].realname = strdup( my_person.realname.c_str() );
      user_list->user_array[index].host = strdup( my_person.host.c_str() );
      user_list->user_array[index].mode = strdup( my_person.mode.c_str() );
      index++;
    }
  }
  return user_list;
}

void
irc_thread_free_users(irc_user_list* user_list)
{
  for (int index = 0; index < user_list->number_user; ++index) {
    free(user_list->user_array[index].user);
    free(user_list->user_array[index].nick);
    free(user_list->user_array[index].realname);
    free(user_list->user_array[index].host);
    free(user_list->user_array[index].mode);
  }
  if (user_list->user_array) {
    free(user_list->user_array);
  }
  free(user_list);
}

};
