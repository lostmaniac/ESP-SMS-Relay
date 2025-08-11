/**
 * @file push_channel_registry.cpp
 * @brief 推送渠道注册器实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 实现推送渠道的动态注册机制
 */

#include "push_channel_registry.h"
#include <algorithm>

/**
 * @brief 获取单例实例
 * @return PushChannelRegistry& 单例引用
 */
PushChannelRegistry& PushChannelRegistry::getInstance() {
    static PushChannelRegistry instance;
    return instance;
}

/**
 * @brief 私有构造函数（单例模式）
 */
PushChannelRegistry::PushChannelRegistry() 
    : debugMode(false) {
    debugPrint("PushChannelRegistry initialized");
}

/**
 * @brief 析构函数
 */
PushChannelRegistry::~PushChannelRegistry() {
    debugPrint("PushChannelRegistry destroyed");
}

/**
 * @brief 注册渠道
 * @param metadata 渠道元数据
 * @return bool 注册是否成功
 */
bool PushChannelRegistry::registerChannel(const ChannelMetadata& metadata) {
    // 检查渠道名称是否有效
    if (!isValidChannelName(metadata.name)) {
        setError("Invalid channel name: " + metadata.name);
        return false;
    }
    
    // 检查渠道是否已存在
    if (findChannel(metadata.name) != nullptr) {
        setError("Channel already exists: " + metadata.name);
        return false;
    }
    
    // 检查别名是否冲突
    for (const String& alias : metadata.aliases) {
        if (findChannel(alias) != nullptr) {
            setError("Channel alias already exists: " + alias);
            return false;
        }
    }
    
    // 检查工厂函数是否有效
    if (!metadata.factory) {
        setError("Invalid factory function for channel: " + metadata.name);
        return false;
    }
    
    // 添加渠道
    channels.push_back(metadata);
    
    debugPrint("Channel registered: " + metadata.name + 
               " (aliases: " + String(metadata.aliases.size()) + ")");
    
    return true;
}

/**
 * @brief 注册渠道（简化版本）
 * @param name 渠道名称
 * @param factory 工厂函数
 * @param aliases 别名列表
 * @return bool 注册是否成功
 */
bool PushChannelRegistry::registerChannel(const String& name, ChannelFactory factory, const std::vector<String>& aliases) {
    ChannelMetadata metadata;
    metadata.name = name;
    metadata.description = "Auto-registered channel: " + name;
    metadata.version = "1.0.0";
    metadata.author = "ESP-SMS-Relay";
    metadata.aliases = aliases;
    metadata.factory = factory;
    
    return registerChannel(metadata);
}

/**
 * @brief 创建渠道实例
 * @param name 渠道名称或别名
 * @return std::unique_ptr<PushChannelBase> 渠道实例
 */
std::unique_ptr<PushChannelBase> PushChannelRegistry::createChannel(const String& name) {
    ChannelMetadata* metadata = findChannel(name);
    if (metadata == nullptr) {
        setError("Channel not found: " + name);
        return nullptr;
    }
    
    try {
        auto channel = metadata->factory();
        if (channel == nullptr) {
            setError("Failed to create channel instance: " + name);
            return nullptr;
        }
        
        debugPrint("Channel created: " + name);
        return channel;
    } catch (const std::exception& e) {
        setError("Exception creating channel " + name + ": " + String(e.what()));
        return nullptr;
    } catch (...) {
        setError("Unknown exception creating channel: " + name);
        return nullptr;
    }
}

/**
 * @brief 检查渠道是否支持
 * @param name 渠道名称或别名
 * @return bool 是否支持
 */
bool PushChannelRegistry::isChannelSupported(const String& name) {
    return findChannel(name) != nullptr;
}

/**
 * @brief 获取所有可用渠道名称
 * @return std::vector<String> 渠道名称列表
 */
std::vector<String> PushChannelRegistry::getAvailableChannels() {
    std::vector<String> names;
    names.reserve(channels.size());
    
    for (const auto& channel : channels) {
        names.push_back(channel.name);
    }
    
    return names;
}

/**
 * @brief 获取渠道元数据
 * @param name 渠道名称或别名
 * @return ChannelMetadata* 元数据指针，如果不存在返回nullptr
 */
const PushChannelRegistry::ChannelMetadata* PushChannelRegistry::getChannelMetadata(const String& name) {
    return findChannel(name);
}

/**
 * @brief 获取所有渠道元数据
 * @return std::vector<ChannelMetadata> 所有渠道元数据
 */
std::vector<PushChannelRegistry::ChannelMetadata> PushChannelRegistry::getAllChannelMetadata() {
    return channels;
}

/**
 * @brief 注销渠道
 * @param name 渠道名称
 * @return bool 注销是否成功
 */
bool PushChannelRegistry::unregisterChannel(const String& name) {
    auto it = std::find_if(channels.begin(), channels.end(),
        [&name](const ChannelMetadata& metadata) {
            return metadata.name == name;
        });
    
    if (it != channels.end()) {
        debugPrint("Channel unregistered: " + name);
        channels.erase(it);
        return true;
    }
    
    setError("Channel not found for unregistration: " + name);
    return false;
}

/**
 * @brief 清空所有注册的渠道
 */
void PushChannelRegistry::clear() {
    size_t count = channels.size();
    channels.clear();
    debugPrint("All channels cleared (" + String(count) + " channels)");
}

/**
 * @brief 获取注册的渠道数量
 * @return size_t 渠道数量
 */
size_t PushChannelRegistry::getChannelCount() {
    return channels.size();
}

/**
 * @brief 设置调试模式
 * @param enabled 是否启用调试模式
 */
void PushChannelRegistry::setDebugMode(bool enabled) {
    debugMode = enabled;
    debugPrint("Debug mode " + String(enabled ? "enabled" : "disabled"));
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String PushChannelRegistry::getLastError() const {
    return lastError;
}

/**
 * @brief 根据名称或别名查找渠道
 * @param name 渠道名称或别名
 * @return ChannelMetadata* 找到的渠道元数据，如果不存在返回nullptr
 */
PushChannelRegistry::ChannelMetadata* PushChannelRegistry::findChannel(const String& name) {
    // 首先按名称查找
    for (auto& channel : channels) {
        if (channel.name == name) {
            return &channel;
        }
    }
    
    // 然后按别名查找
    for (auto& channel : channels) {
        for (const String& alias : channel.aliases) {
            if (alias == name) {
                return &channel;
            }
        }
    }
    
    return nullptr;
}

/**
 * @brief 检查渠道名称是否有效
 * @param name 渠道名称
 * @return bool 是否有效
 */
bool PushChannelRegistry::isValidChannelName(const String& name) {
    if (name.isEmpty()) {
        return false;
    }
    
    // 检查是否包含无效字符
    for (int i = 0; i < name.length(); i++) {
        char c = name.charAt(i);
        if (!isalnum(c) && c != '_' && c != '-') {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void PushChannelRegistry::setError(const String& error) {
    lastError = error;
    debugPrint("Error: " + error);
}

/**
 * @brief 调试输出
 * @param message 调试信息
 */
void PushChannelRegistry::debugPrint(const String& message) {
    if (debugMode) {
        Serial.println("[PushChannelRegistry] " + message);
    }
}