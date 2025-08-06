/**
 * @file filesystem_manager.cpp
 * @brief 文件系统管理器实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "filesystem_manager.h"
#include <Arduino.h>

/**
 * @brief 构造函数
 */
FilesystemManager::FilesystemManager() {
    status = FILESYSTEM_NOT_INITIALIZED;
    debugMode = false;
    initialized = false;
    lastError = "";
    
    // 初始化文件系统信息
    fsInfo.totalBytes = 0;
    fsInfo.usedBytes = 0;
    fsInfo.freeBytes = 0;
    fsInfo.usagePercent = 0.0;
    fsInfo.mounted = false;
}

/**
 * @brief 析构函数
 */
FilesystemManager::~FilesystemManager() {
    if (status == FILESYSTEM_READY) {
        unmount();
    }
}

/**
 * @brief 获取单例实例
 * @return FilesystemManager& 单例引用
 */
FilesystemManager& FilesystemManager::getInstance() {
    static FilesystemManager instance;
    return instance;
}

/**
 * @brief 初始化文件系统
 * @param formatOnFail 初始化失败时是否格式化
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool FilesystemManager::initialize(bool formatOnFail) {
    if (initialized && status == FILESYSTEM_READY) {
        debugPrint("文件系统已经初始化");
        return true;
    }
    
    debugPrint("开始初始化LittleFS文件系统...");
    status = FILESYSTEM_INITIALIZING;
    
    // 尝试挂载LittleFS
    if (!mount()) {
        if (formatOnFail) {
            debugPrint("挂载失败，尝试格式化文件系统...");
            if (!format()) {
                setError("文件系统格式化失败");
                status = FILESYSTEM_ERROR;
                return false;
            }
            
            // 格式化后重新挂载
            if (!mount()) {
                setError("格式化后挂载失败");
                status = FILESYSTEM_ERROR;
                return false;
            }
        } else {
            setError("文件系统挂载失败，且未启用格式化选项");
            status = FILESYSTEM_ERROR;
            return false;
        }
    }
    
    // 更新文件系统信息
    updateFilesystemInfo();
    
    initialized = true;
    status = FILESYSTEM_READY;
    
    debugPrint("LittleFS文件系统初始化成功");
    debugPrint("总空间: " + String(fsInfo.totalBytes) + " 字节");
    debugPrint("已使用: " + String(fsInfo.usedBytes) + " 字节");
    debugPrint("可用空间: " + String(fsInfo.freeBytes) + " 字节");
    debugPrint("使用率: " + String(fsInfo.usagePercent, 1) + "%");
    
    return true;
}

/**
 * @brief 挂载文件系统到根目录
 * @return true 挂载成功
 * @return false 挂载失败
 */
bool FilesystemManager::mount() {
    debugPrint("正在挂载LittleFS文件系统...");
    
    // 挂载LittleFS到根目录，不自动格式化
    if (!LittleFS.begin(false)) {
        setError("LittleFS挂载失败");
        fsInfo.mounted = false;
        return false;
    }
    
    fsInfo.mounted = true;
    debugPrint("LittleFS文件系统挂载成功");
    return true;
}

/**
 * @brief 卸载文件系统
 * @return true 卸载成功
 * @return false 卸载失败
 */
bool FilesystemManager::unmount() {
    debugPrint("正在卸载LittleFS文件系统...");
    
    LittleFS.end();
    fsInfo.mounted = false;
    status = FILESYSTEM_NOT_INITIALIZED;
    initialized = false;
    
    debugPrint("LittleFS文件系统卸载完成");
    return true;
}

/**
 * @brief 格式化文件系统
 * @return true 格式化成功
 * @return false 格式化失败
 */
bool FilesystemManager::format() {
    debugPrint("开始格式化LittleFS文件系统...");
    status = FILESYSTEM_FORMATTING;
    
    // 如果已挂载，先卸载
    if (fsInfo.mounted) {
        LittleFS.end();
        fsInfo.mounted = false;
    }
    
    // 格式化LittleFS
    if (!LittleFS.format()) {
        setError("LittleFS格式化失败");
        status = FILESYSTEM_ERROR;
        return false;
    }
    
    debugPrint("LittleFS文件系统格式化完成");
    return true;
}

/**
 * @brief 获取文件系统状态
 * @return FilesystemStatus 当前状态
 */
FilesystemStatus FilesystemManager::getStatus() const {
    return status;
}

/**
 * @brief 获取文件系统信息
 * @return FilesystemInfo 文件系统信息
 */
FilesystemInfo FilesystemManager::getFilesystemInfo() {
    if (status == FILESYSTEM_READY) {
        updateFilesystemInfo();
    }
    return fsInfo;
}

/**
 * @brief 检查文件系统是否就绪
 * @return true 文件系统就绪
 * @return false 文件系统未就绪
 */
bool FilesystemManager::isReady() const {
    return (status == FILESYSTEM_READY && fsInfo.mounted);
}

/**
 * @brief 检查文件是否存在
 * @param path 文件路径
 * @return true 文件存在
 * @return false 文件不存在
 */
bool FilesystemManager::fileExists(const String& path) {
    if (!isReady()) {
        setError("文件系统未就绪");
        return false;
    }
    
    return LittleFS.exists(path);
}

/**
 * @brief 创建目录
 * @param path 目录路径
 * @return true 创建成功
 * @return false 创建失败
 */
bool FilesystemManager::createDirectory(const String& path) {
    if (!isReady()) {
        setError("文件系统未就绪");
        return false;
    }
    
    // LittleFS支持真正的目录结构
    if (path.length() == 0 || !path.startsWith("/")) {
        setError("无效的目录路径");
        return false;
    }
    
    // 创建目录
    if (!LittleFS.mkdir(path)) {
        // 检查目录是否已存在
        File dir = LittleFS.open(path);
        if (dir && dir.isDirectory()) {
            dir.close();
            debugPrint("目录已存在: " + path);
            return true;
        }
        dir.close();
        setError("创建目录失败: " + path);
        return false;
    }
    
    debugPrint("目录创建成功: " + path);
    return true;
}

/**
 * @brief 删除文件
 * @param path 文件路径
 * @return true 删除成功
 * @return false 删除失败
 */
bool FilesystemManager::deleteFile(const String& path) {
    if (!isReady()) {
        setError("文件系统未就绪");
        return false;
    }
    
    if (!LittleFS.exists(path)) {
        setError("文件不存在: " + path);
        return false;
    }
    
    if (!LittleFS.remove(path)) {
        setError("删除文件失败: " + path);
        return false;
    }
    
    debugPrint("文件删除成功: " + path);
    return true;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String FilesystemManager::getLastError() const {
    return lastError;
}

/**
 * @brief 设置调试模式
 * @param enabled 是否启用调试
 */
void FilesystemManager::setDebugMode(bool enabled) {
    debugMode = enabled;
}

/**
 * @brief 获取文件系统对象引用
 * @return fs::FS& 文件系统引用
 */
fs::FS& FilesystemManager::getFS() {
    return LittleFS;
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void FilesystemManager::setError(const String& error) {
    lastError = error;
    if (debugMode) {
        Serial.println("[FilesystemManager] 错误: " + error);
    }
}

/**
 * @brief 调试输出
 * @param message 调试信息
 */
void FilesystemManager::debugPrint(const String& message) {
    if (debugMode) {
        Serial.println("[FilesystemManager] " + message);
    }
}

/**
 * @brief 更新文件系统信息
 */
void FilesystemManager::updateFilesystemInfo() {
    if (!fsInfo.mounted) {
        fsInfo.totalBytes = 0;
        fsInfo.usedBytes = 0;
        fsInfo.freeBytes = 0;
        fsInfo.usagePercent = 0.0;
        return;
    }
    
    fsInfo.totalBytes = LittleFS.totalBytes();
    fsInfo.usedBytes = LittleFS.usedBytes();
    fsInfo.freeBytes = fsInfo.totalBytes - fsInfo.usedBytes;
    
    if (fsInfo.totalBytes > 0) {
        fsInfo.usagePercent = (float)fsInfo.usedBytes / (float)fsInfo.totalBytes * 100.0;
    } else {
        fsInfo.usagePercent = 0.0;
    }
}