
#ifndef ARTESTCONFIG_H
#define ARTESTCONFIG_H

struct ArConfigPrimitiveData;

struct ArConfigDisplayHintData;


class ArConfig;

class ArTestConfig
{
public:
  
	ArTestConfig(ArConfig *config,
                const char *configSectionName);

	virtual ~ArTestConfig();

protected:

  bool addToConfig(ArConfig *config,
                   const char *configSectionName);

private:

  ArConfigPrimitiveData *myTopLevelConfigPrimitives;

  ArConfigPrimitiveData *myListMemberPrimitives;

  ArConfigDisplayHintData *myDisplayHintData;

}; // end class ArTestConfig

#endif // ARTESTCONFIG_H