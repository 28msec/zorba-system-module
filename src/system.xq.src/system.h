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

#ifndef __COM_ZORBA_WWW_MODULES_SYSTEM_H__
#define __COM_ZORBA_WWW_MODULES_SYSTEM_H__
#include <vector>
#include <map>

#include <zorba/zorba.h>
#include <zorba/external_module.h>
#include <zorba/function.h>

namespace zorba { namespace system {
  class SystemModule : public ExternalModule {
    protected:
      static zorba::Item globalOSName;
      static zorba::Item globalOSNodeName;
      static zorba::Item globalOSVersionMajor;
      static zorba::Item globalOSVersionMinor;
      static zorba::Item globalOSVersionBuild;
      static zorba::Item globalOSVersionRelease;
      static zorba::Item globalOSVersionVersion;
      static zorba::Item globalOSVersion;
      static zorba::Item globalOSArch;
      static zorba::Item globalOSis64;
      static zorba::Item globalHWLogicalCPU;
      static zorba::Item globalHWPhysicalCPU;
      static zorba::Item globalHWLogicalPerPhysicalCPU;
      static zorba::Item globalHWPhysicalMemory;
      static zorba::Item globalHWVirtualMemory;
      static zorba::Item globalHWManufacturer;
      static zorba::Item globalLinuxDistributor;
      static zorba::Item globalLinuxDistributorVersion;
      static zorba::Item globalUserName;
      static zorba::Item globalZorbaModulePath;
      static zorba::Item globalZorbaVersion;
      static zorba::Item globalZorbaVersionMajor;
      static zorba::Item globalZorbaVersionMinor;
      static zorba::Item globalZorbaVersionPatch;
    private:
      ExternalFunction* thePropertyFunction;
      ExternalFunction* thePropertiesFunction;
      const static String SYSTEM_MODULE_NAMESPACE;
    public:
      enum GLOBAL_KEY { OS_NAME, OS_NODE_NAME, OS_VER_MAJOR, OS_VER_MINOR,
                        OS_VER_BUILD, OS_VER_RELEASE, OS_VER_VERSION, OS_VER,
                        OS_ARCH, OS_IS64, HARDWARE_lOGICAL_CPU, HARDWARE_PHYSICAL_CPU,
                        HARDWARE_LOGICAL_PER_PHYSICAL_CPU, HARDWARE_PHYSICAL_MEMORY,
                        HARDWARE_VIRTUAL_MEMORY, HARDWARE_MANUFACTURER, LINUX_DISTRIBUTOR,
                        LINUX_DISTRIBUTOR_VERSION, USER_NAME, ZORBA_MODULE_PATH, ZORBA_VER, ZORBA_VER_MAJOR,
                        ZORBA_VER_MINOR, ZORBA_VER_PATCH };
                        
      SystemModule();
      virtual ~SystemModule();
      
      virtual String getURI() const { return SYSTEM_MODULE_NAMESPACE; }

      virtual ExternalFunction* getExternalFunction(const String& localName);

      virtual void destroy();
      
      static zorba::Item& getGlobalKey(enum GLOBAL_KEY g);
  };

  class SystemFunction {
    protected:
      const ExternalModule* theModule;
      ItemFactory* theFactory;
      std::map<String, String> theProperties;
    public:
      SystemFunction(const ExternalModule* aModule);
    protected:
      String getURI() const { return theModule->getURI(); }
      bool getEnv(const String& name, String& value) const;
      void getEnvNames(std::vector<Item>& names) const;
      String intToString(int v);
  };

  class PropertiesFunction : public NonContextualExternalFunction, public SystemFunction {
    public:
      PropertiesFunction(const ExternalModule* mod) : SystemFunction(mod) {}

      virtual String getLocalName() const { return "properties"; }

      virtual ItemSequence_t 
      evaluate(const ExternalFunction::Arguments_t& args) const;
      virtual String getURI() const { return SystemFunction::getURI(); }
  };

  class PropertyFunction : public ContextualExternalFunction, public SystemFunction {
    public:
      PropertyFunction(const ExternalModule* mod) : SystemFunction(mod) {}

      virtual String getLocalName() const { return "property"; }

      virtual ItemSequence_t 
      evaluate(const ExternalFunction::Arguments_t& args,
               const StaticContext* sctx,
               const DynamicContext* dctx) const;
      virtual String getURI() const { return SystemFunction::getURI(); }
  };

} } // namespace zorba, namespace system

#ifdef WIN32
#  define DLL_EXPORT __declspec(dllexport)
#else
#  define DLL_EXPORT __attribute__ ((visibility("default")))
#endif

extern "C" DLL_EXPORT zorba::ExternalModule* createModule() {
  return new zorba::system::SystemModule();
}

#endif // __COM_ZORBA_WWW_MODULES_SYSTEM_H__
