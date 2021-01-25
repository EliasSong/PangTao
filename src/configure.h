#pragma once
#include <memory>
#include <sstream>
#include <string>
#include "log.h"
#include <boost/lexical_cast.hpp>
namespace PangTao
{
    class ConfigureVarBase
    {
    public:
        typedef std::shared_ptr<ConfigureVarBase> ptr;
        ConfigureVarBase(const std::string &name, const std::string &desc = "")
        {
            m_name = name;
            m_desc = desc;
        }
        virtual ~ConfigureVarBase() {}
        const std::string &getName() const { return m_name; }
        const std::string &getDesc() const { return m_desc; }
        virtual std::string toString() = 0;
        virtual bool fromString(const std::string &str) = 0;

    protected:
        std::string m_name;
        std::string m_desc;
    };
    template <class T>
    class ConfigureVar : public ConfigureVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;
        ConfigureVar(const T &default_val, const std::string &name, const std::string &desc = "", )
            : ConfigureVarBase(name, desc)
        {
            m_val = default_val;
        }
        std::string toString() override
        {
            try
            {
                return boost::lexical_cast<std::string>(m_val);
            }
            catch (std::exception &e)
            {
                PANGTAO_LOG_ERROR(PangTao::LoggerManager::getInstance()->getRoot(),
                                  "ConfigureVal::toString exception " + e.what() + " convert: " + typeid(m_val).name() + " to string");
            }
            return "";
        }
        bool fromString(const std::string &str) override
        {
            try
            {
                m_val = boost::lexical_cast<T>(str);
                return true;
            }
            catch (std::exception &e)
            {
                PANGTAO_LOG_ERROR(PangTao::LoggerManager::getInstance()->getRoot(),
                                  "ConfigureVal::toString exception " + e.what() + " convert: string to " + typeid(m_val).name());
            }
            return false;
        }

    private:
        T m_val;
    };
    class Configure
    {
    public:
        template <class T>
        static typename ConfigureVar<T>::ptr Lookup(const std::string &name, const T &default_value, const std::string &desc = "")
        {
            auto i = Lookup<T>(name);
            if (i)
            {
                PANGTAO_LOG_INFO(PangTao::LoggerManager::getInstance()->getRoot(),
                "Lookup name = "+"existed"
                );
            }
        }
        template <class T>
        static typename ConfigureVar<T>::ptr Lookup(const std::string &name)
        {
            auto i = m_configureVarMap.find(name);
            if (i == m_configureVarMap.end())
            {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigureVar<T>>(i->second);
        }

    private:
        static std::unordered_map<std::string, ConfigureVarBase::ptr> m_configureVarMap;
    };
} // namespace PangTao