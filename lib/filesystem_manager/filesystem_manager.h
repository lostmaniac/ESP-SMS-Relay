/**
 * @file filesystem_manager.h
 * @brief 文件系统管理器头文件
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. SPIFFS文件系统的初始化和挂载
 * 2. 文件系统状态监控
 * 3. 文件系统操作的统一接口
 * 4. 文件系统错误处理
 */

#ifndef FILESYSTEM_MANAGER_H
#define FILESYSTEM_MANAGER_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <FS.h>

/**
 * @enum FilesystemStatus
 * @brief 文件系统状态枚举
 */
enum FilesystemStatus {
    FILESYSTEM_NOT_INITIALIZED,    ///< 文件系统未初始化
    FILESYSTEM_INITIALIZING,       ///< 文件系统正在初始化
    FILESYSTEM_READY,              ///< 文件系统就绪
    FILESYSTEM_ERROR,              ///< 文件系统错误
    FILESYSTEM_FORMATTING          ///< 文件系统正在格式化
};

/**
 * @struct FilesystemInfo
 * @brief 文件系统信息结构体
 */
struct FilesystemInfo {
    size_t totalBytes;      ///< 总空间大小（字节）
    size_t usedBytes;       ///< 已使用空间大小（字节）
    size_t freeBytes;       ///< 可用空间大小（字节）
    float usagePercent;     ///< 使用率百分比
    bool mounted;           ///< 是否已挂载
};

/**
 * @class FilesystemManager
 * @brief 文件系统管理器类
 * 
 * 负责SPIFFS文件系统的统一管理和操作
 */
class FilesystemManager {
public:
    /**
     * @brief 获取单例实例
     * @return FilesystemManager& 单例引用
     */
    static FilesystemManager& getInstance();

    /**
     * @brief 初始化文件系统
     * @param formatOnFail 初始化失败时是否格式化
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize(bool formatOnFail = true);

    /**
     * @brief 挂载文件系统到根目录
     * @return true 挂载成功
     * @return false 挂载失败
     */
    bool mount();

    /**
     * @brief 卸载文件系统
     * @return true 卸载成功
     * @return false 卸载失败
     */
    bool unmount();

    /**
     * @brief 格式化文件系统
     * @return true 格式化成功
     * @return false 格式化失败
     */
    bool format();

    /**
     * @brief 获取文件系统状态
     * @return FilesystemStatus 当前状态
     */
    FilesystemStatus getStatus() const;

    /**
     * @brief 获取文件系统信息
     * @return FilesystemInfo 文件系统信息
     */
    FilesystemInfo getFilesystemInfo();

    /**
     * @brief 检查文件系统是否就绪
     * @return true 文件系统就绪
     * @return false 文件系统未就绪
     */
    bool isReady() const;

    /**
     * @brief 检查文件是否存在
     * @param path 文件路径
     * @return true 文件存在
     * @return false 文件不存在
     */
    bool fileExists(const String& path);

    /**
     * @brief 创建目录
     * @param path 目录路径
     * @return true 创建成功
     * @return false 创建失败
     */
    bool createDirectory(const String& path);

    /**
     * @brief 删除文件
     * @param path 文件路径
     * @return true 删除成功
     * @return false 删除失败
     */
    bool deleteFile(const String& path);

    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError() const;

    /**
     * @brief 设置调试模式
     * @param enabled 是否启用调试
     */
    void setDebugMode(bool enabled);

    /**
     * @brief 获取文件系统对象引用
     * @return fs::FS& 文件系统引用
     */
    fs::FS& getFS();

private:
    /**
     * @brief 构造函数（私有）
     */
    FilesystemManager();

    /**
     * @brief 析构函数
     */
    ~FilesystemManager();

    /**
     * @brief 禁用拷贝构造函数
     */
    FilesystemManager(const FilesystemManager&) = delete;

    /**
     * @brief 禁用赋值操作符
     */
    FilesystemManager& operator=(const FilesystemManager&) = delete;

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

    /**
     * @brief 更新文件系统信息
     */
    void updateFilesystemInfo();

private:
    FilesystemStatus status;        ///< 文件系统状态
    String lastError;              ///< 最后的错误信息
    bool debugMode;                ///< 调试模式
    bool initialized;              ///< 是否已初始化
    FilesystemInfo fsInfo;         ///< 文件系统信息
};

#endif // FILESYSTEM_MANAGER_H