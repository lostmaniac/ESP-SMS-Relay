#ifndef HTML_CONTENT_H
#define HTML_CONTENT_H

const char HTML_CONTENT[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP 短信转发配置</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <nav>
        <div class="nav-container">
            <h1>ESP 短信转发器</h1>
            <div class="nav-links">
                <a href="#" onclick="showPage('rules')">转发规则</a>
                <a href="#" onclick="showPage('sms_history')">短信历史</a>
                <a href="#" onclick="showPage('logs')">系统日志</a>
                <a href="#" onclick="showPage('status')">系统状态</a>
                <a href="#" onclick="showPage('database')">数据库工具</a>
                <a href="#" onclick="showPage('help')">配置指南</a>
                <a href="#" onclick="showPage('wifi_settings')">WiFi设置</a>
            </div>
        </div>
    </nav>

    <main id="content"></main>

    <!-- Rule Edit Modal -->
    <div id="rule-modal" class="modal">
        <div class="modal-content">
            <span class="close-button" onclick="closeModal()">&times;</span>
            <h2 id="modal-title">添加规则</h2>
            <form id="rule-form" onsubmit="return saveRule()">
                <input type="hidden" id="rule-id">
                <label for="rule-name">名称:</label>
                <input type="text" id="rule-name" required>

                <label for="source-number">来源号码 (* 代表所有):</label>
                <input type="text" id="source-number" required>

                <label for="keywords">关键字 (逗号分隔):</label>
                <input type="text" id="keywords">

                <label for="push-type">推送类型:</label>
                <select id="push-type"></select>

                <label for="push-config">推送配置 (JSON):</label>
                <textarea id="push-config" rows="6" required></textarea>

                <label><input type="checkbox" id="enabled"> 启用</label>
                <label><input type="checkbox" id="is-default-forward"> 默认转发</label>

                <button type="submit">保存</button>
            </form>
        </div>
    </div>

    <script src="/script.js"></script>
</body>
</html>
)rawliteral";

#endif // HTML_CONTENT_H