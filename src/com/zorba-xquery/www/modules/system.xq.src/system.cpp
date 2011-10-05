/*
 * Copyright 2006-2008 The FLWOR Foundation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cstdlib>
#include <cstdio>
#include <sstream>

#ifdef WIN32
# include <Windows.h>
# include <malloc.h>
# include <stdio.h>
# include <tchar.h>
# include <winreg.h>
#else
#include <sys/utsname.h>
# ifndef __APPLE__
#   include <sys/sysinfo.h>
# else
#   include <sys/param.h>
#   include <sys/sysctl.h>
# endif
#endif

#include <zorba/zorba_string.h>
#include <zorba/singleton_item_sequence.h>
#include <zorba/vector_item_sequence.h>
#include <zorba/empty_sequence.h>
#include <zorba/item_factory.h>


#ifdef LINUX
#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
extern char** environ;
#elif defined APPLE
# include <crt_externs.h>
#endif

#include "system.h"


namespace zorba { namespace system {

  const String SystemModule::SYSTEM_MODULE_NAMESPACE = "http://www.zorba-xquery.com/modules/system";

#ifdef WIN32
  typedef BOOL (WINAPI *LPFN_GLPI)(
      PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
      PDWORD);

  // Helper function to count set bits in the processor mask.
  DWORD CountSetBits(ULONG_PTR bitMask)
  {
    DWORD LSHIFT = sizeof(ULONG_PTR)*8 - 1;
    DWORD bitSetCount = 0;
    ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
    DWORD i;
    for (i = 0; i <= LSHIFT; ++i)
    {
      bitSetCount += ((bitMask & bitTest)?1:0);
      bitTest/=2;
    }

    return bitSetCount;
  }

  DWORD numaNodeCount = 0;
  DWORD processorPackageCount = 0;
  DWORD logicalProcessorCount = 0;
  DWORD processorCoreCount = 0;
  DWORD processorL1CacheCount = 0;
  DWORD processorL2CacheCount = 0;
  DWORD processorL3CacheCount = 0;
  static void countProcessors() {
    LPFN_GLPI glpi;
    BOOL done = FALSE;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
    DWORD returnLength = 0;
    DWORD byteOffset = 0;
    PCACHE_DESCRIPTOR Cache;

    glpi = (LPFN_GLPI) GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation");
    if (NULL == glpi) {
      // GetLogicalProcessorInformation is not supported.
      return;
    }

    while (!done)
    {
      DWORD rc = glpi(buffer, &returnLength);
      if (FALSE == rc)
      {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
          if (buffer)
            free(buffer);
          buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnLength);
          if (NULL == buffer)
          {
            // Error: Allocation failure
            return;
          }
        } else {
          // Error %d, GetLastError()
          return;
        }
      } else {
        done = TRUE;
      }
    }
    ptr = buffer;
    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
    {
      switch (ptr->Relationship)
      {
        case RelationNumaNode:
          // Non-NUMA systems report a single record of this type.
          numaNodeCount++;
          break;
        case RelationProcessorCore:
          processorCoreCount++;
          // A hyperthreaded core supplies more than one logical processor.
          logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
          break;

        case RelationCache:
          // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache.
          Cache = &ptr->Cache;
          if (Cache->Level == 1)
          {
            processorL1CacheCount++;
          }
          else if (Cache->Level == 2)
          {
            processorL2CacheCount++;
          }
          else if (Cache->Level == 3)
          {
            processorL3CacheCount++;
          }
          break;
        case RelationProcessorPackage:
          // Logical processors share a physical package.
          processorPackageCount++;
          break;
        default:
          // Error: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.
          break;
      }
      byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
      ptr++;
    }
    free(buffer);
    return;
  }

#endif

#ifdef LINUX
  static void trim(std::string& str, char delim)
  {
    std::string::size_type pos = str.find_last_not_of(delim);
    if(pos != std::string::npos) {
      str.erase(pos + 1);
      pos = str.find_first_not_of(delim);
      if(pos != std::string::npos) str.erase(0, pos);
    }
    else str.erase(str.begin(), str.end());
  }


  int logical=0;
  int cores=0;
  int physical=0;


  static void countProcessors() {
    logical=0;
    cores=0;
    physical=0;

    std::ifstream in("/proc/cpuinfo");
    if(in) {
      std::string name;
      std::string value;

      while(in) {
        getline(in, name, ':');
        trim (name, ' ');
        trim (name, '\t');
        trim (name, '\n');
        getline(in, value);
        trim (value, ' ');
        trim (value, '\t');
        if (name == "processor") {
          logical++;
        }

        if (name == "cpu cores") {
          cores = atoi(value.c_str());
        }
      }
      physical = logical/cores;
      in.close();
    }
  }



  static std::pair<std::string, std::string> getDistribution() {
    std::pair<std::string, std::string> lRes;
    FILE *pipe;
    const char* command = "lsb_release -r -i";
    pipe = (FILE*) popen(command, "r");

    char line[1024];
    while (fgets(line, sizeof(line), pipe)) {
      std::stringstream s(line);
      std::string name, value;
      getline(s, name, ':');
      trim(name, ' ');
      trim(name, '\t');
      getline(s, value, ':');
      trim(value, ' ');
      trim(value, '\t');
      trim(value, '\n');
      if (name == "Distributor ID") {
        lRes.first = value;
      } else {
        lRes.second = value;
      }
    }
    return lRes;
  }
#endif

  SystemModule::SystemModule()
    : thePropertyFunction(0), thePropertiesFunction(0)
  {
  }

  ExternalFunction* SystemModule::getExternalFunction(const String& localName) {
    if (localName == "properties") {
      if (!thePropertiesFunction)
        thePropertiesFunction = new PropertiesFunction(this);
      return thePropertiesFunction;
    } else if (localName == "property") {
      if (!thePropertyFunction)
        thePropertyFunction = new PropertyFunction(this);
      return thePropertyFunction;
    }
    return 0;
  }

  void SystemModule::destroy() {
    delete this;
  }

  SystemModule::~SystemModule() {
    delete thePropertyFunction;
    delete thePropertiesFunction;
  }


  SystemFunction::SystemFunction(const ExternalModule* aModule)
    : theModule(aModule), theFactory(Zorba::getInstance(0)->getItemFactory())
  {
#ifdef WIN32

    {
      DWORD nodeNameLength = MAX_COMPUTERNAME_LENGTH + 1;
      TCHAR nodeName[MAX_COMPUTERNAME_LENGTH + 1];
      char nodeNameC[MAX_COMPUTERNAME_LENGTH + 1];
      GetComputerName(nodeName, &nodeNameLength);
      for (DWORD i = 0; i < nodeNameLength; ++i) {
        nodeNameC[i] = static_cast<char>(nodeName[i]);
      }
      nodeNameC[nodeNameLength] = NULL;  // Terminate string
      theProperties.insert(std::make_pair("os.node.name", nodeNameC));
    }

    {
      DWORD dwVersion = 0;
      DWORD dwMajorVersion = 0;
      DWORD dwMinorVersion = 0;
      DWORD dwBuild = 0;

      dwVersion = GetVersion();

      // Get the Windows version.
      dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
      dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

      // Get the build number.
      if (dwVersion < 0x80000000)
        dwBuild = (DWORD)(HIWORD(dwVersion));

      std::string major;
      std::string minor;
      std::string build;
      {
        std::stringstream sMajor;
        sMajor << dwMajorVersion;
        std::stringstream sMinor;
        sMinor << dwMinorVersion;
        std::stringstream sBuild;
        sBuild << dwBuild;

        major = sMajor.str();
        minor = sMinor.str();
        build = sBuild.str();
      }
      theProperties.insert(std::make_pair("os.version.major", major));
      theProperties.insert(std::make_pair("os.version.minor", minor));
      theProperties.insert(std::make_pair("os.version.build", build));
      theProperties.insert(std::make_pair("os.version", major + "." + minor + "." + build));
      // http://msdn.microsoft.com/en-us/library/ms724832(v=VS.85).aspx
      std::string operativeSystem;
      theProperties.insert(std::make_pair("os.name", "Windows"));
      {
        countProcessors();
        std::stringstream logicalProcessors;
        logicalProcessors << processorPackageCount;
        std::stringstream physicalProcessors;
        physicalProcessors << logicalProcessorCount;
        std::stringstream logicalPerPhysicalProcessors;
        logicalPerPhysicalProcessors << (logicalProcessorCount / processorPackageCount );
        theProperties.insert(std::make_pair("hardware.physical.cpu", logicalProcessors.str() ));
        theProperties.insert(std::make_pair("hardware.logical.cpu", physicalProcessors.str() ));
        theProperties.insert(std::make_pair("hardware.logical.per.physical.cpu", logicalPerPhysicalProcessors.str() ));
      }
      {
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof (statex);
        GlobalMemoryStatusEx (&statex);
        std::stringstream virtualMemory;
        virtualMemory << statex.ullTotalVirtual;
        std::stringstream physicalMemory;
        physicalMemory << statex.ullTotalPhys;
        theProperties.insert(std::make_pair("hardware.virtual.memory", virtualMemory.str() ));
        theProperties.insert(std::make_pair("hardware.physical.memory", physicalMemory.str() ));
      }

    }
    {
      DWORD userNameLength = 1023;
      TCHAR userName[1024];
      char userNameC[1024];
      GetUserName(userName, &userNameLength);
      for (DWORD i = 0; i < userNameLength; ++i) {
        userNameC[i] = static_cast<char>(userName[i]);
      }
      theProperties.insert(std::make_pair("user.name", userNameC));
    }
    {
      SYSTEM_INFO info;
      GetSystemInfo(&info);
      if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        theProperties.insert(std::make_pair("os.arch", "x86_64"));
        theProperties.insert(std::make_pair("os.is64", "true"));
      } else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
        theProperties.insert(std::make_pair("os.arch", "ia64"));
        theProperties.insert(std::make_pair("os.is64", "true"));
      } else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        theProperties.insert(std::make_pair("os.arch", "i386"));
        theProperties.insert(std::make_pair("os.is64", "false"));
      }
    }

    {
      HKEY keyHandle;
      TCHAR value [1024];
      char valueC [1024];
      DWORD size = 0;
      DWORD Type;
      if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_QUERY_VALUE, &keyHandle) == ERROR_SUCCESS)
      {
        RegQueryValueEx( keyHandle, L"SystemManufacturer", NULL, &Type, (LPBYTE)value, &size);
        for (DWORD i = 0; i < size; ++i) {
          valueC[i] = static_cast<char>(value[i]);
        }
        if (size > 0)
          theProperties.insert(std::make_pair("hardware.manufacturer", valueC));
      }
      RegCloseKey(keyHandle);
    }


#else
    struct utsname osname;
    if (uname(&osname) == 0)
    {
      theProperties.insert(std::make_pair("os.name", osname.sysname));
      theProperties.insert(std::make_pair("os.node.name", osname.nodename));
      theProperties.insert(std::make_pair("os.version.release", osname.release));
      theProperties.insert(std::make_pair("os.version.version", osname.version));
      theProperties.insert(std::make_pair("os.version", osname.release));
      theProperties.insert(std::make_pair("os.arch", osname.machine));
    }
    char* lUser = getenv("USER");
    if (lUser)
    {
      theProperties.insert(std::make_pair("user.name", lUser));
    }
    theProperties.insert(std::make_pair("os.is64", "false"));
    {
#ifdef __APPLE__
      int mib[2];
      size_t len = 4;
      uint32_t res = 0;

      mib[0] = CTL_HW;
      mib[1] = HW_NCPU;
      sysctl(mib, 2, &res, &len, NULL, NULL);
      std::stringstream lStream;
      lStream << res;
      theProperties.insert(std::make_pair("hardware.physical.cpu", lStream.str()));
#else
      countProcessors();
      std::stringstream logicalProcessor;
      std::stringstream physicalProcessor;
      std::stringstream logicalPerPhysicalProcessors;
      logicalProcessor << logical;
      physicalProcessor << physical;
      logicalPerPhysicalProcessors << cores;
      theProperties.insert(std::make_pair("hardware.logical.per.physical.cpu", logicalPerPhysicalProcessors.str() ));
      theProperties.insert(std::make_pair("hardware.physical.cpu", physicalProcessor.str() ));
      theProperties.insert(std::make_pair("hardware.logical.cpu", logicalProcessor.str() ));
#endif
    }
    {
# ifdef LINUX
      struct sysinfo sys_info;
      if(sysinfo(&sys_info) == 0) {
        std::stringstream memory;
        memory << sys_info.totalram;
        std::stringstream swap;
        swap << sys_info.totalswap;
        theProperties.insert(std::make_pair("hardware.virtual.memory", swap.str() ));
        theProperties.insert(std::make_pair("hardware.physical.memory", memory.str() ));
      }
# elif defined __APPLE__
      int mib[2];
      size_t len = 8;
      uint64_t res = 0;

      mib[0] = CTL_HW;
      mib[1] = HW_MEMSIZE;
      sysctl(mib, 2, &res, &len, NULL, NULL);
      std::stringstream lStream;
      lStream << res;
      theProperties.insert(std::make_pair("hardware.physical.memory", lStream.str()));
# endif
    }

#endif
#ifdef LINUX
    theProperties.insert(std::make_pair("linux.distributor", ""));
    theProperties.insert(std::make_pair("linux.distributor.version", ""));
#endif
    theProperties.insert(std::make_pair("zorba.version", Zorba::version().getVersion()));
    theProperties.insert(std::make_pair("zorba.version.major", intToString(Zorba::version().getMajorVersion())));
    theProperties.insert(std::make_pair("zorba.version.minor", intToString(Zorba::version().getMinorVersion())));
    theProperties.insert(std::make_pair("zorba.version.patch", intToString(Zorba::version().getPatchVersion())));
  }

  String SystemFunction::intToString(int v) {
    std::stringstream ss;
    ss << v;
    return ss.str();
  }

  bool SystemFunction::getEnv(const String& name, String& value) const
  {
    char* v = getenv(name.c_str());
    if (v == NULL) return false;
    value = v;
    return true;
  }

  void SystemFunction::getEnvNames(std::vector<Item>& names) const
  {
#ifdef WIN32
    // put in the environment variables
    TCHAR *l_EnvStr;
    l_EnvStr = GetEnvironmentStrings();

    LPTSTR l_str = l_EnvStr;

    int count = 0;
    while (true)
    {
      if (*l_str == 0) break;
      while (*l_str != 0) l_str++;
      l_str++;
      count++;
    }

    for (int i = 0; i < count; i++)
    {
      char lStr[1024];
      memset(lStr, 0, 1024);
      for (int i =0; i<1023 && l_EnvStr[i]; ++i) {
        lStr[i] = (char) l_EnvStr[i];
      }
      std::string e(lStr);
      std::string name("env.");
      name += e.substr(0, e.find('='));
      String value = e.substr(e.find('=') + 1);
      if (name != "env.")
        names.push_back(theFactory->createString(name));
      while(*l_EnvStr != '\0')
        l_EnvStr++;
      l_EnvStr++;
    }
    //FreeEnvironmentStrings(l_EnvStr);
#else
# ifdef APPLE
    char** environ = *_NSGetEnviron();
# endif // APPLE
    for (int i = 0; environ[i] != NULL; ++i) {
      std::string e(environ[i]);
      String name("env.");
      name += e.substr(0, e.find('='));
      names.push_back(theFactory->createString(name));
    }
#endif
  }

  ItemSequence_t PropertiesFunction::evaluate(
      const ExternalFunction::Arguments_t& args) const {
    std::vector<Item> lRes;
    getEnvNames(lRes);
    for (std::map<String, String>::const_iterator i = theProperties.begin();
        i != theProperties.end(); ++i) {
      Item lItem = theFactory->createString(i->first.c_str());
      lRes.push_back(lItem);
    }
    // insert the zorba module path
    lRes.push_back(theFactory->createString("zorba.module.path"));
    return ItemSequence_t(new VectorItemSequence(lRes));
  }

  ItemSequence_t PropertyFunction::evaluate(
      const ExternalFunction::Arguments_t& args,
      const StaticContext* sctx,
      const DynamicContext* dctx) const {
    Item item;
    Iterator_t arg0_iter = args[0]->getIterator();
    arg0_iter->open();
    arg0_iter->next(item);
    arg0_iter->close();
    String envS = item.getStringValue();
    String lRes;
    if (envS == "zorba.module.path") {
      std::vector<String> lModulePaths;
      sctx->getFullModulePaths(lModulePaths);
      if (lModulePaths.size() == 0)
        return ItemSequence_t(new SingletonItemSequence(theFactory->createString("")));
      lRes = lModulePaths[0];
      for (std::vector<String>::iterator i = lModulePaths.begin() + 1; i != lModulePaths.end(); ++i) {
#ifdef WIN32
        lRes += ";";
#else
        lRes += ":";
#endif
        lRes += *i;
      }
    } else if (envS.substr(0,4) == "env.") {
      //Sleep(5000);
      if (!getEnv(envS.substr(4), lRes)) {
        return ItemSequence_t(new EmptySequence());
      }
#ifdef LINUX
    } else if (envS == "linux.distributor") {
      lRes = getDistribution().first;
    } else if (envS == "linux.distributor.version") {
      lRes = getDistribution().second;
#endif
    } else {
      std::map<String, String>::const_iterator i;
      if ((i = theProperties.find(envS.c_str())) != theProperties.end()) {
        lRes = i->second;
      } else {
        return ItemSequence_t(new EmptySequence());
      }
    }
    return ItemSequence_t(new SingletonItemSequence(theFactory->createString(lRes)));
  }
}} // namespace zorba, system

