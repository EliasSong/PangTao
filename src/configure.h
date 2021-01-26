#pragma once
#include <memory>
#include <sstream>
#include <string>
#include "log.h"
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <unordered_map>
#include <functional>
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
        typedef std::shared_ptr<ConfigureVar> ptr;
        typedef std::function<void(const T &old_value, const T &new_value)> configureVarChangedCallback;
        ConfigureVar(const T &default_val, const std::string &name, const std::string &desc = "")
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
                                  "ConfigureVal::toString exception " + std::string(e.what()) + " convert: " + std::string(typeid(m_val).name()) + " to string");
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
                                  "ConfigureVal::toString exception " + std::string(e.what()) + " convert: string to " + std::string(typeid(m_val).name()));
            }
            return false;
        }
        const T getVal() const
        {

            return m_val;
        }
        void setVal(T val)
        {
            if (val == m_val)
            {
                return;
            }
            for (auto &i : m_cbs)
            {
                i.second(m_val, val);
            }
            m_val = val;
        }
        void addListener(uint64_t key, configureVarChangedCallback cb)
        {
            m_cbs[key] = cb;
        }
        void removeListener(uint64_t key)
        {
            m_cbs.erase(key);
        }
        void clearListener()
        {
            m_cbs.clear();
        }
        configureVarChangedCallback getListener(uint64_t key)
        {
            if (m_cbs.find(key) != m_cbs.end())
            {
                return m_cbs[key];
            }
            return nullptr;
        }

    private:
        T m_val;
        std::unordered_map<uint64_t, configureVarChangedCallback> m_cbs;
    };
    class Configure
    {
    public:
        template <class T>
        static typename ConfigureVar<T>::ptr Lookup(const T &default_value, const std::string &name, const std::string &desc = "")
        {

            auto i = Lookup<T>(name);

            if (i)
            {

                PANGTAO_LOG_INFO(PangTao::LoggerManager::getInstance()->getRoot(), "Lookup name = " + name + "existed");
            }
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._0123456789") != std::string::npos)
            {
                PANGTAO_LOG_ERROR(PangTao::LoggerManager::getInstance()->getRoot(), "Lookup name invalid " + name);
                throw std::invalid_argument(name);
            }

            typename ConfigureVar<T>::ptr v(new ConfigureVar<T>(default_value, name, desc));

            m_configureVarMap[name] = v;

            return v;
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