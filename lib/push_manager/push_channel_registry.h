/**
 * @file push_channel_registry.h
 * @brief 推送渠道注册器接口
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 定义推送渠道的动态注册机制，支持渠道的自动发现和注册
 */

#ifndef PUSH_CHANNEL_REGISTRY_H
#define PUSH_CHANNEL_REGISTRY_H

#include <Arduino.h>
#include <memory>
#include <functional>
#include <vector>
#include "push_channel_base.h"

/**
 * @class PushChannelRegistry
 * @brief 推送渠道注册器
 * 
 * 提供渠道的动态注册机制，支持：
 * - 渠道自动注册
 * - 渠道工厂函数管理
 * - 渠道别名支持
 * - 渠道元数据管理
 */
class PushChannelRegistry {
public:
    /**
     * @brief 渠道工厂函数类型
     */
    using ChannelFactory = std::function<std::unique_ptr<PushChannelBase>()>;
    
    /**
     * @brief 渠道元数据结构
     */
    struct ChannelMetadata {
        String name;            ///< 渠道名称
        String description;     ///< 渠道描述
        String version;         ///< 渠道版本
        String author;          ///< 渠道作者
        std::vector<String> aliases;  ///< 渠道别名
        ChannelFactory factory; ///< 工厂函数
    };
    
    /**
     * @brief 获取单例实例
     * @return PushChannelRegistry& 单例引用
     */
    static PushChannelRegistry& getInstance();
    
    /**
     * @brief 注册渠道
     * @param metadata 渠道元数据
     * @return bool 注册是否成功
     */
    bool registerChannel(const ChannelMetadata& metadata);
    
    /**
     * @brief 注册渠道（简化版本）
     * @param name 渠道名称
     * @param factory 工厂函数
     * @param aliases 别名列表
     * @return bool 注册是否成功
     */
    bool registerChannel(const String& name, ChannelFactory factory, const std::vector<String>& aliases = {});
    
    /**
     * @brief 创建渠道实例
     * @param name 渠道名称或别名
     * @return std::unique_ptr<PushChannelBase> 渠道实例
     */
    std::unique_ptr<PushChannelBase> createChannel(const String& name);
    
    /**
     * @brief 检查渠道是否支持
     * @param name 渠道名称或别名
     * @return bool 是否支持
     */
    bool isChannelSupported(const String& name);
    
    /**
     * @brief 获取所有可用渠道名称
     * @return std::vector<String> 渠道名称列表
     */
    std::vector<String> getAvailableChannels();
    
    /**
     * @brief 获取渠道元数据
     * @param name 渠道名称或别名
     * @return ChannelMetadata* 元数据指针，如果不存在返回nullptr
     */
    const ChannelMetadata* getChannelMetadata(const String& name);
    
    /**
     * @brief 获取所有渠道元数据
     * @return std::vector<ChannelMetadata> 所有渠道元数据
     */
    std::vector<ChannelMetadata> getAllChannelMetadata();
    
    /**
     * @brief 注销渠道
     * @param name 渠道名称
     * @return bool 注销是否成功
     */
    bool unregisterChannel(const String& name);
    
    /**
     * @brief 清空所有注册的渠道
     */
    void clear();
    
    /**
     * @brief 获取注册的渠道数量
     * @return size_t 渠道数量
     */
    size_t getChannelCount();
    
    /**
     * @brief 设置调试模式
     * @param enabled 是否启用调试模式
     */
    void setDebugMode(bool enabled);
    
    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError() const;

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    PushChannelRegistry();
    
    /**
     * @brief 析构函数
     */
    ~PushChannelRegistry();
    
    /**
     * @brief 禁用拷贝构造函数
     */
    PushChannelRegistry(const PushChannelRegistry&) = delete;
    
    /**
     * @brief 禁用赋值操作符
     */
    PushChannelRegistry& operator=(const PushChannelRegistry&) = delete;
    
    /**
     * @brief 根据名称或别名查找渠道
     * @param name 渠道名称或别名
     * @return ChannelMetadata* 找到的渠道元数据，如果不存在返回nullptr
     */
    ChannelMetadata* findChannel(const String& name);
    
    /**
     * @brief 检查渠道名称是否有效
     * @param name 渠道名称
     * @return bool 是否有效
     */
    bool isValidChannelName(const String& name);
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);
    
    /**
     * @brief 调试输出
     * @param message 调试信息
     */
    void debugPrint(const String& message);

private:
    std::vector<ChannelMetadata> channels;  ///< 注册的渠道列表
    bool debugMode;                         ///< 调试模式
    String lastError;                       ///< 最后的错误信息
};

/**
 * @brief 渠道自动注册器宏
 * 
 * 用于在渠道文件中自动注册渠道，使用方法：
 * REGISTER_PUSH_CHANNEL("wechat", WechatChannel, {"企业微信", "微信"});
 */
#define REGISTER_PUSH_CHANNEL(name, class_name, aliases) \
    namespace { \
        class class_name##Registrar { \
        public: \
            class_name##Registrar() { \
                registerChannel(); \
            } \
            static void registerChannel() { \
                static bool registered = false; \
                if (!registered) { \
                    PushChannelRegistry::getInstance().registerChannel( \
                        name, \
                        []() -> std::unique_ptr<PushChannelBase> { \
                            return std::unique_ptr<PushChannelBase>(new class_name()); \
                        }, \
                        aliases \
                    ); \
                    registered = true; \
                } \
            } \
        }; \
        static class_name##Registrar class_name##_registrar_instance; \
    } \
    static void force_##class_name##_registration() { \
        class_name##Registrar::registerChannel(); \
    }

#endif // PUSH_CHANNEL_REGISTRY_H