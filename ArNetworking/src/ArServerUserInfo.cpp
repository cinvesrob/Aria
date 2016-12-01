#include "Aria.h"
#include "ArExport.h"
#include "ArServerUserInfo.h"
#include "md5.h"

AREXPORT ArServerUserInfo::ArServerUserInfo(const char *baseDirectory) :
  myV1HeaderCB(this, &ArServerUserInfo::v1HeaderCallback),
  myV1UserCB(this, &ArServerUserInfo::v1UserCallback),
  myV1DoNotUseCB(this, &ArServerUserInfo::v1DoNotUseCallback)
{
  myDataMutex.setLogName("ArServerUserInfo::myDataMutex");
  myParser.setBaseDirectory(baseDirectory);
  myGotHeader = false;
  myDoNotUse = false;
  myLogFailureVerbosely = false;
}

AREXPORT ArServerUserInfo::~ArServerUserInfo()
{

}

AREXPORT void ArServerUserInfo::setBaseDirectory(const char *baseDirectory)
{
  myDataMutex.lock();
  myParser.setBaseDirectory(baseDirectory);
  myDataMutex.unlock();
}


bool ArServerUserInfo::v1HeaderCallback(ArArgumentBuilder * arg)
{
  removeHandlers();
  if (myParser.addHandler("user", &myV1UserCB) && 
      myParser.addHandler("doNotUse", &myV1DoNotUseCB))
  {
    myGotHeader = true;
    myDoNotUse = false;
    return true;
  }
  else
  {
    ArLog::log(ArLog::Terse, "ArServerUserInfo::Could not add user info callback");
    return false;
  }
}

bool ArServerUserInfo::v1UserCallback(ArArgumentBuilder * arg)
{
  if (arg->getArgc() < 2)
  {
    ArLog::log(ArLog::Normal, "ArServerUserInfo: Not enough arguments to user info 'user <user> <password> <reapingOptional:group>");
    return false;
  }

  const char *user;
  const char *password;
  user = arg->getArg(0);
  if (!ArUtil::isOnlyAlphaNumeric(user))
  {
    ArLog::log(ArLog::Normal, "ArServerUserInfo: user '%s' has non-alpha-numeric characters in it", user);
    return false;
  }
  password = arg->getArg(1);
  if (!ArUtil::isOnlyAlphaNumeric(password))
  {
    ArLog::log(ArLog::Normal, "ArServerUserInfo: password has non-alpha-numeric characters in it");
    return false;
  }
  
  if (myPasswords.find(user) != myPasswords.end())
  {
    ArLog::log(ArLog::Terse, "ArServerUserInfo: Already a user with name %s", user);
    return false;
  }
  //printf("Adding user %s with password %s and groups\n", user, password);
  myPasswords[user] = password;
  
  std::set<std::string, ArStrCaseCmpOp> *groups;
  groups = new std::set<std::string, ArStrCaseCmpOp>;
  myGroups[user] = groups;
  
  unsigned int i;
  for (i = 2; i < arg->getArgc(); i++)
  {
    groups->insert(arg->getArg(i));
  }
  return true;
}

bool ArServerUserInfo::v1DoNotUseCallback(ArArgumentBuilder * arg)
{
  myDoNotUse = true;
  return true;
}

void ArServerUserInfo::removeHandlers(void)
{
  myParser.remHandler(&myV1HeaderCB);
  myParser.remHandler(&myV1UserCB);
}

AREXPORT bool ArServerUserInfo::readFile(const char *fileName)
{
  myDataMutex.lock();
  removeHandlers();
  myPasswords.clear();
  ArUtil::deleteSetPairs(myGroups.begin(), myGroups.end());
  myGroups.clear();
  myParser.addHandler("UserInfoVersion1", &myV1HeaderCB);
  myGotHeader = false;
  if (!myParser.parseFile(fileName, false))
  {
    myDataMutex.unlock();
    ArLog::log(ArLog::Terse, "Problem loading server user info");
    return false;
  }
  removeHandlers();
  if (myGotHeader)
  {
    myDataMutex.unlock();
    ArLog::log(ArLog::Verbose, "Loaded server user info");
    return true;
  }
  else
  {
    myDataMutex.unlock();
    ArLog::log(ArLog::Normal, "Problem loading server user info: no header");
    return false;
  }
}

AREXPORT bool ArServerUserInfo::matchUserPassword(
	const char *user, unsigned char password[16], const char *passwordKey,
	const char *serverKey, bool logFailureVerbosely) const
{
  std::map<std::string, std::string, ArStrCaseCmpOp>::const_iterator it;
  if (!ArUtil::isOnlyAlphaNumeric(user))
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerUserInfo: login denied: user '%s' has non alpha-numeric characters", 
	       user);
    return false;
  }
  it = myPasswords.find(user);
  if (it == myPasswords.end())
  {
    ArLog::log(ArLog::Normal, "ArServerUserInfo: login denied: No user %s", 
	       user);
    return false;
  }
  /*  thd old simple clear text way
  if (ArUtil::strcmp((*it).second, password) != 0)
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerUserInfo: login denied: Wrong password for user %s", 
	       user);
    return false;
    }*/
  md5_state_t md5State;
  unsigned char md5Digest[16];

  md5_init(&md5State);
  md5_append(&md5State, (unsigned char *)serverKey, strlen(serverKey));
  md5_append(&md5State, (unsigned char *)passwordKey, strlen(passwordKey));
  md5_append(&md5State, (unsigned char *)(*it).second.c_str(), 
	     (*it).second.size());
  md5_finish(&md5State, md5Digest);
  
  if (memcmp(md5Digest, password, 16) != 0)
  {
    
    ArLog::log(ArLog::Normal, 
	       "ArServerUserInfo: login denied: Wrong password for user %s", 
	       user);
    if (logFailureVerbosely)
    {
      ArLog::log(ArLog::Terse, "The server key is '%s'", serverKey);
      ArLog::log(ArLog::Terse, "The password seed is '%s'", passwordKey);
      ArLog::log(ArLog::Terse, "The resulting digest is: ");
      logDigest(md5Digest);
      ArLog::log(ArLog::Terse, "The returned password is:");
      logDigest(password);
    }
    return false;
  }
  return true;
}

AREXPORT std::set<std::string, ArStrCaseCmpOp> 
ArServerUserInfo::getUsersGroups(const char *user) const
{
  std::map<std::string, std::set<std::string, ArStrCaseCmpOp> *, ArStrCaseCmpOp>::const_iterator sit;
  std::set<std::string, ArStrCaseCmpOp> ret;
  
  if ((sit = myGroups.find(user)) == myGroups.end())
  {
    ArLog::log(ArLog::Terse, "No user %s", user);
    return ret;
  }
  
  ret = *((*sit).second);
  return ret;
}

AREXPORT void ArServerUserInfo::logUsers(void) const
{
  std::map<std::string, std::string, ArStrCaseCmpOp>::const_iterator it;
  std::set<std::string, ArStrCaseCmpOp> groups;
  std::set<std::string, ArStrCaseCmpOp>::const_iterator git;
  std::string line;
  if (myDoNotUse)
    ArLog::log(ArLog::Terse, "The user info should not be used");
  else
    ArLog::log(ArLog::Terse, "The user info should be used");
  for (it = myPasswords.begin(); it != myPasswords.end(); it++)
  {
    line = "User ";
    line += (*it).first;
    line += " has groups:";

    groups = getUsersGroups((*it).first.c_str());
      /*
    if ((sit = myGroups.find((*it).first)) == myGroups.end())
    {
      ArLog::log(ArLog::Terse, "ArServerUserInfo::logUsersAndGroups: Hideous error with the groups of user '%s'", (*it).first.c_str());
      continue;
    } 
    groups = (*sit).second;
      */
    for (git = groups.begin(); git != groups.end(); git++)
    {
      line += " ";
      line += (*git);
    }
    ArLog::log(ArLog::Terse, line.c_str());
  }
}

AREXPORT bool ArServerUserInfo::doNotUse(void) const
{
  return myDoNotUse;
}


void ArServerUserInfo::logDigest(unsigned char digest[16]) const
{
  int i;
  char buf[1024];
  buf[0] = '\0';
  int highNibble;
  char highChar;
  int lowNibble;
  char lowChar;
  for (i = 0; i < 16; i++)
  {
    highNibble = ((unsigned char)digest[i] >> 4) & 0xf;
    if (highNibble <= 9)
      highChar = '0' + highNibble;
    else
      highChar = 'a' + highNibble - 10;
    lowNibble = (unsigned char)digest[i] & 0xf;
    if (lowNibble <= 9)
      lowChar = '0' + lowNibble;
    else
      lowChar = 'a' + lowNibble - 10;
    sprintf(buf, "%s%c%c ", buf, highChar, lowChar);
  }
  ArLog::log(ArLog::Terse, buf);
}



