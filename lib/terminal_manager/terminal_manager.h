/**
 * @file terminal_manager.h
 * @brief 终端管理器主接口 - 提供SMS转发规则的数据库配置管理功能
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 终端管理器是一个轻量级模块，专注于提供SMS转发规则的数据库配置管理功能。
 * 该模块通过简洁的接口封装数据库操作，为系统提供转发规则的增删改查功能。
 */

#ifndef TERMINAL_MANAGER_H
#define TERMINAL_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include "forward_rule_manager.h"

/**
 * @enum TerminalError
 * @brief 终端管理器错误类型
 */
enum TerminalError {
    TERMINAL_SUCCESS = 0,          ///< 成功
    TERMINAL_ERROR_NOT_INITIALIZED, ///< 未初始化
    TERMINAL_ERROR_DATABASE,       ///< 数据库错误
    TERMINAL_ERROR_INVALID_PARAM,  ///< 无效参数
    TERMINAL_ERROR_RULE_NOT_FOUND, ///< 规则不存在
    TERMINAL_ERROR_RULE_EXISTS,    ///< 规则已存在
    TERMINAL_ERROR_VALIDATION,     ///< 验证失败
    TERMINAL_ERROR_UNKNOWN         ///< 未知错误
};

/**
 * @struct TerminalConfig
 * @brief 终端管理器配置
 */
struct TerminalConfig {
    bool enabled;                  ///< 模块启用
    int maxRules;                 ///< 最大规则数量
    bool enableCache;             ///< 启用缓存
    int cacheSize;                ///< 缓存大小
    bool enableValidation;        ///< 启用验证
    bool enableLogging;           ///< 启用日志
};

/**
 * @class TerminalManager
 * @brief 终端管理器类 - 提供统一的转发规则管理接口
 * 
 * 该类负责：
 * - 提供统一的转发规则管理接口
 * - 协调数据库操作
 * - 处理错误和异常
 * - 管理模块生命周期
 * - 提供CLI命令行接口
 */
class TerminalManager {
public:
    /**
     * @brief 获取单例实例
     * @return TerminalManager& 单例引用
     */
    static TerminalManager& getInstance();
    
    /**
     * @brief 初始化终端管理器
     * @return bool 成功返回true，失败返回false
     */
    bool initialize();
    
    /**
     * @brief 清理资源
     */
    void cleanup();
    
    // ==================== 转发规则管理接口 ====================
    
    /**
     * @brief 添加转发规则
     * @param rule 规则对象
     * @return int 规则ID，失败返回-1
     */
    int addForwardRule(const ForwardRule& rule);
    
    /**
     * @brief 更新转发规则
     * @param rule 规则对象（必须包含有效的ID）
     * @return bool 成功返回true，失败返回false
     */
    bool updateForwardRule(const ForwardRule& rule);
    
    /**
     * @brief 删除转发规则
     * @param ruleId 规则ID
     * @return bool 成功返回true，失败返回false
     */
    bool deleteForwardRule(int ruleId);
    
    /**
     * @brief 获取单个转发规则
     * @param ruleId 规则ID
     * @return ForwardRule 规则对象，失败时ID为-1
     */
    ForwardRule getForwardRule(int ruleId);
    
    /**
     * @brief 获取转发规则列表
     * @param condition 查询条件
     * @return std::vector<ForwardRule> 规则列表
     */
    std::vector<ForwardRule> getForwardRules(const RuleQueryCondition& condition = {});
    
    // ==================== 规则状态管理接口 ====================
    
    /**
     * @brief 启用规则
     * @param ruleId 规则ID
     * @return bool 成功返回true，失败返回false
     */
    bool enableRule(int ruleId);
    
    /**
     * @brief 禁用规则
     * @param ruleId 规则ID
     * @return bool 成功返回true，失败返回false
     */
    bool disableRule(int ruleId);
    
    /**
     * @brief 设置规则优先级
     * @param ruleId 规则ID
     * @param priority 优先级（数字越小优先级越高）
     * @return bool 成功返回true，失败返回false
     */
    bool setRulePriority(int ruleId, int priority);
    
    // ==================== 规则测试和验证接口 ====================
    
    /**
     * @brief 测试规则是否匹配
     * @param ruleId 规则ID
     * @param sender 发送者号码
     * @param content 短信内容
     * @return bool 匹配返回true，不匹配返回false
     */
    bool testRule(int ruleId, const String& sender, const String& content);
    
    /**
     * @brief 验证规则配置是否有效
     * @param rule 规则对象
     * @return bool 有效返回true，无效返回false
     */
    bool validateRuleConfig(const ForwardRule& rule);
    
    // ==================== 统计信息接口 ====================
    
    /**
     * @brief 获取规则总数
     * @return int 规则总数
     */
    int getRuleCount();
    
    /**
     * @brief 获取启用规则数量
     * @return int 启用规则数量
     */
    int getEnabledRuleCount();
    
    /**
     * @brief 获取最常用的规则
     * @param limit 限制数量
     * @return std::vector<ForwardRule> 最常用规则列表
     */
    std::vector<ForwardRule> getMostUsedRules(int limit = 10);
    
    // ==================== 批量操作接口 ====================
    
    /**
     * @brief 启用所有规则
     * @return bool 成功返回true，失败返回false
     */
    bool enableAllRules();
    
    /**
     * @brief 禁用所有规则
     * @return bool 成功返回true，失败返回false
     */
    bool disableAllRules();
    
    /**
     * @brief 删除所有规则
     * @return bool 成功返回true，失败返回false
     */
    bool deleteAllRules();
    
    /**
     * @brief 导入规则
     * @param rules 规则列表
     * @return bool 成功返回true，失败返回false
     */
    bool importRules(const std::vector<ForwardRule>& rules);
    
    /**
     * @brief 导出规则
     * @return std::vector<ForwardRule> 所有规则列表
     */
    std::vector<ForwardRule> exportRules();
    
    // ==================== CLI命令行接口 ====================
    
    /**
     * @brief 处理串口命令输入
     * @param command 命令字符串
     * @return bool 成功处理返回true
     */
    bool processCommand(const String& command);
    
    /**
     * @brief 启动CLI命令行模式
     */
    void startCLI();
    
    /**
     * @brief 停止CLI命令行模式
     */
    void stopCLI();
    
    /**
     * @brief 检查CLI是否运行
     * @return bool 运行中返回true
     */
    bool isCLIRunning();
    
    /**
     * @brief 处理串口输入循环
     */
    void handleSerialInput();
    
    // ==================== 错误处理接口 ====================
    
    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError();
    
private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    TerminalManager();
    
    /**
     * @brief 私有析构函数
     */
    ~TerminalManager();
    
    // 禁止拷贝构造和赋值
    TerminalManager(const TerminalManager&) = delete;
    TerminalManager& operator=(const TerminalManager&) = delete;
    
    // ==================== CLI命令处理私有方法 ====================
    
    /**
     * @brief 解析命令行
     * @param command 命令字符串
     * @param args 解析后的参数列表
     * @return String 命令名称
     */
    String parseCommand(const String& command, std::vector<String>& args);
    
    /**
     * @brief 解析命名参数命令行
     * @param command 命令字符串
     * @param namedArgs 解析后的命名参数映射
     * @param positionalArgs 解析后的位置参数列表
     * @return String 命令名称
     */
    String parseNamedCommand(const String& command, std::map<String, String>& namedArgs, std::vector<String>& positionalArgs);
    

    
    /**
     * @brief 使用命名参数执行添加规则命令
     * @param args 参数列表
     */
    void executeAddCommandWithNamedParams(const std::vector<String>& args);
    
    /**
     * @brief 添加规则并显示结果
     * @param rule 规则对象
     */
    void addRuleAndShowResult(const ForwardRule& rule);
    
    /**
     * @brief 执行帮助命令
     * @param args 参数列表
     */
    void executeHelpCommand(const std::vector<String>& args);
    
    /**
     * @brief 执行规则列表命令
     * @param args 参数列表
     */
    void executeListCommand(const std::vector<String>& args);
    
    /**
     * @brief 执行添加规则命令
     * @param args 参数列表
     */
    void executeAddCommand(const std::vector<String>& args);
    
    /**
     * @brief 执行删除规则命令
     * @param args 参数列表
     */
    void executeDeleteCommand(const std::vector<String>& args);
    
    /**
     * @brief 执行启用规则命令
     * @param args 参数列表
     */
    void executeEnableCommand(const std::vector<String>& args);
    
    /**
     * @brief 执行禁用规则命令
     * @param args 参数列表
     */
    void executeDisableCommand(const std::vector<String>& args);
    
    /**
     * @brief 执行测试规则命令
     * @param args 参数列表
     */
    void executeTestCommand(const std::vector<String>& args);
    
    /**
     * @brief 执行状态查询命令
     * @param args 参数列表
     */
    void executeStatusCommand(const std::vector<String>& args);
    
    /**
     * @brief 执行导入命令
     * @param args 参数列表
     */
    void executeImportCommand(const std::vector<String>& args);
    
    /**
     * @brief 执行导出命令
     * @param args 参数列表
     */
    void executeExportCommand(const std::vector<String>& args);
    
    /**
     * @brief 打印命令提示符
     */
    void printPrompt();
    
    /**
     * @brief 打印欢迎信息
     */
    void printWelcome();
    
    /**
     * @brief 打印规则信息
     * @param rule 规则对象
     */
    void printRule(const ForwardRule& rule);
    
    /**
     * @brief 打印规则列表
     * @param rules 规则列表
     */
    void printRules(const std::vector<ForwardRule>& rules);
    
    /**
     * @brief 显示可用的推送渠道
     */
    void showAvailableChannels();
    
    /**
     * @brief 显示特定渠道的详细配置帮助
     * @param channelName 渠道名称
     */
    void showChannelConfigHelp(const String& channelName);
    
    /**
     * @brief 生成推送渠道帮助信息
     * @return String 动态生成的推送渠道帮助信息
     */
    String generateChannelHelp();
    
    /**
     * @brief 生成推送渠道配置示例
     * @return String 动态生成的配置示例
     */
    String generateChannelExamples();
    
    // ==================== 私有成员变量 ====================
    
    bool initialized;                    ///< 初始化状态
    bool cliRunning;                    ///< CLI运行状态
    String lastError;                  ///< 最后的错误信息
    String inputBuffer;                ///< 输入缓冲区
    ForwardRuleManager* ruleManager;   ///< 转发规则管理器
    TerminalConfig config;             ///< 终端配置
    
    // 默认配置
    static const TerminalConfig DEFAULT_CONFIG;
};

#endif // TERMINAL_MANAGER_H