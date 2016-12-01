#ifndef ARSERVERUSERINFO_H
#define ARSERVERUSERINFO_H

#include "Aria.h"

/// This class holds information about users and loads it from a file
/**
   For a description of the algorithm used to match passwords and keys
   and all look at the documentation for ArServerBase.

   The file format for this class is set up to be easy to make a new
   version of and yet read all the old versions.

   For all of the versions everything after ; or # is ignored.  The
   version information is then the first line of non comments.

   The first version is described as such: The version string is
   '<code>UserInfoVersion1</code>'.  Then there are lines that follow for each user
   which are '<code>user</code> <i>userName password groups</i>'.  The passwords are
   plain text in the file, though they aren't sent that way over the
   network (look at ArServerBase docs for details).  
   To display the groups available use ArServerBase::logCommandGroups().

   There is an example user info file in ArNetworking/examples/serverDemo.userInfo

**/
class ArServerUserInfo
{
public:
  /// Constructor
  AREXPORT ArServerUserInfo(const char *baseDirectory = NULL);
  /// Destructor
  AREXPORT ~ArServerUserInfo();
  /// Loads the file, returns false if it wasn't there
  AREXPORT bool readFile(const char *fileName);
  /// Sets the base directory
  AREXPORT void setBaseDirectory(const char *baseDirectory);
  /// Matchs a user and password, false if user or password is wrong
  AREXPORT bool matchUserPassword(const char *user, unsigned char password[16],
				  const char *passwordKey, 
				  const char *serverKey,
				  bool logFailureVerbosely = false) const;
  AREXPORT bool doNotUse(void) const;
  /// Gets the groups a user is in (returns empty set if no user)
  AREXPORT std::set<std::string, ArStrCaseCmpOp> getUsersGroups(
	  const char *user) const;
  /// Logs the users and groups
  AREXPORT void logUsers(void) const;
protected:
  bool v1HeaderCallback(ArArgumentBuilder * arg);
  bool v1UserCallback(ArArgumentBuilder * arg);
  bool v1DoNotUseCallback(ArArgumentBuilder * arg);
  void removeHandlers(void);
  void logDigest(unsigned char digest[16]) const;

  ArMutex myDataMutex;
  std::map<std::string, std::string, ArStrCaseCmpOp> myPasswords;
  std::map<std::string, std::set<std::string, ArStrCaseCmpOp> *, 
      ArStrCaseCmpOp> myGroups;
  ArRetFunctor1C<bool, ArServerUserInfo, ArArgumentBuilder *> myV1HeaderCB;
  ArRetFunctor1C<bool, ArServerUserInfo, ArArgumentBuilder *> myV1UserCB;
  ArRetFunctor1C<bool, ArServerUserInfo, ArArgumentBuilder *> myV1DoNotUseCB;
  std::string myBaseDirectory;
  ArFileParser myParser;
  bool myGotHeader;
  bool myDoNotUse;
  bool myLogFailureVerbosely;

};

#endif // ARSERVERUSERINFO_H
