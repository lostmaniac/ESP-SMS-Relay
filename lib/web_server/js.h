#ifndef JS_CONTENT_H
#define JS_CONTENT_H

const char JS_CONTENT[] PROGMEM = R"rawliteral(
let currentRules = [];
const SMS_PAGE_SIZE = 20;

window.onload = () => {
    showPage('rules');
};

function showPage(page) {
    const content = document.getElementById('content');
    content.innerHTML = '<h2>加载中...</h2>';
    if (page === 'rules') {
        loadRules();
    } else if (page === 'sms_history') {
        loadSmsHistory();
    } else if (page === 'logs') {
        loadLogs();
    } else if (page === 'status') {
        loadStatus();
    } else if (page === 'database') {
        loadDatabase();
    } else if (page === 'help') {
        loadHelpPage();
    } else if (page === 'wifi_settings') {
        loadWifiSettingsPage();
    }
}

async function loadWifiSettingsPage() {
    const content = document.getElementById('content');
    try {
        const response = await fetch('/api/wifi/ap_settings');
        const data = await response.json();
        let html = '<h2>AP热点设置</h2>';
        html += '<div class="form-group">';
        html += '    <label for="ssid">AP SSID:</label>';
        html += `    <input type="text" id="ssid" value="${data.ssid}">`;
        html += '</div>';
        html += '<div class="form-group">';
        html += '    <label for="password">AP密码 (至少8位):</label>';
        html += `    <input type="text" id="password" value="${data.password}">`;
        html += '</div>';
        html += '<div class="form-group">';
        html += '    <label for="channel">信道 (1-13):</label>';
        html += `    <input type="number" id="channel" min="1" max="13" value="${data.channel}">`;
        html += '</div>';
        html += '<div class="form-group">';
        html += '    <label for="maxConnections">最大连接数 (1-8):</label>';
        html += `    <input type="number" id="maxConnections" min="1" max="8" value="${data.maxConnections}">`;
        html += '</div>';
        html += '<button onclick="saveWifiSettings()">保存并重启</button>';
        html += '<div id="message" class="message"></div>';
        content.innerHTML = html;
    } catch (error) {
        console.error('加载AP设置失败:', error);
        content.innerHTML = '<h2>加载AP设置失败，请重试。</h2>';
    }
}

async function saveWifiSettings() {
    const ssid = document.getElementById('ssid').value;
    const password = document.getElementById('password').value;
    const channel = parseInt(document.getElementById('channel').value);
    const maxConnections = parseInt(document.getElementById('maxConnections').value);
    const messageDiv = document.getElementById('message');

    if (password.length > 0 && password.length < 8) {
        messageDiv.className = 'message error';
        messageDiv.innerText = 'AP密码必须至少8位。';
        return;
    }

    if (channel < 1 || channel > 13) {
        messageDiv.className = 'message error';
        messageDiv.innerText = '信道必须在1-13之间。';
        return;
    }

    if (maxConnections < 1 || maxConnections > 8) {
        messageDiv.className = 'message error';
        messageDiv.innerText = '最大连接数必须在1-8之间。';
        return;
    }

    try {
        const response = await fetch('/api/wifi/ap_settings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ ssid, password, channel, maxConnections, enabled: true })
        });

        if (response.ok) {
            messageDiv.className = 'message success';
            messageDiv.innerText = 'AP设置已保存，设备正在重启...';
        } else {
            const errorText = await response.text();
            messageDiv.className = 'message error';
            messageDiv.innerText = '保存AP设置失败: ' + errorText;
        }
    } catch (error) {
        console.error('保存AP设置时出错:', error);
        messageDiv.className = 'message error';
        messageDiv.innerText = '保存AP设置时发生错误。';
    }
}

async function loadHelpPage() {
    const content = document.getElementById('content');
    try {
        const response = await fetch('/api/docs/forward_rules');
        const text = await response.text();
        content.innerHTML = `<h2>配置指南</h2><div class="docs-container"><pre>${text}</pre></div>`;
    } catch (error) {
        console.error('加载文档失败:', error);
        content.innerHTML = '<h2>加载文档失败，请重试。</h2>';
    }
}

async function loadRules() {
    try {
        const response = await fetch('/api/rules');
        currentRules = await response.json();
        const content = document.getElementById('content');
        let html = '<h2>转发规则 <button onclick="openModal()">添加规则</button></h2>';
        html += '<table><thead><tr><th>ID</th><th>名称</th><th>来源号码</th><th>关键字</th><th>推送类型</th><th>启用</th><th>操作</th></tr></thead><tbody>';
        currentRules.forEach(rule => {
            html += `<tr>
                <td>${rule.id}</td>
                <td>${rule.rule_name}</td>
                <td>${rule.source_number}</td>
                <td>${rule.keywords}</td>
                <td>${rule.push_type}</td>
                <td>${rule.enabled ? '是' : '否'}</td>
                <td class="action-buttons">
                    <button onclick="openModal(${rule.id})">编辑</button>
                    <button onclick="deleteRule(${rule.id})">删除</button>
                </td>
            </tr>`;
        });
        html += '</tbody></table>';
        content.innerHTML = html;
    } catch (error) {
        console.error('加载规则失败:', error);
        document.getElementById('content').innerHTML = '<h2>加载规则失败，请重试。</h2>';
    }
}

async function loadSmsHistory(page = 1) {
    const content = document.getElementById('content');
    try {
        const response = await fetch(`/api/sms_history?page=${page}&limit=${SMS_PAGE_SIZE}`);
        const data = await response.json();
        let html = '<h2>短信历史</h2>';
        html += '<table><thead><tr><th>ID</th><th>发送方</th><th>内容</th><th>接收时间</th><th>状态</th></tr></thead><tbody>';
        data.records.forEach(sms => {
            html += `<tr>
                <td>${sms.id}</td>
                <td>${sms.from}</td>
                <td class="sms-content">${sms.content}</td>
                <td>${new Date(sms.received_at * 1000).toLocaleString()}</td>
                <td>${sms.status}</td>
            </tr>`;
        });
        html += '</tbody></table>';
        html += renderPagination(page, data.total, SMS_PAGE_SIZE, 'loadSmsHistory');
        content.innerHTML = html;
    } catch (error) {
        console.error('加载短信历史失败:', error);
        content.innerHTML = '<h2>加载短信历史失败，请重试。</h2>';
    }
}

function renderPagination(currentPage, totalItems, pageSize, clickHandler) {
    const totalPages = Math.ceil(totalItems / pageSize);
    if (totalPages <= 1) return '';

    let html = '<div class="pagination">';
    
    const isFirstPage = currentPage === 1;
    html += `<a href="#" onclick="${isFirstPage ? 'return false;' : `${clickHandler}(${currentPage - 1})`}" class="${isFirstPage ? 'disabled' : ''}">&laquo; 上一页</a>`;

    for (let i = 1; i <= totalPages; i++) {
        html += `<a href="#" onclick="${clickHandler}(${i})" class="${i === currentPage ? 'active' : ''}">${i}</a>`;
    }

    const isLastPage = currentPage >= totalPages;
    html += `<a href="#" onclick="${isLastPage ? 'return false;' : `${clickHandler}(${currentPage + 1})`}" class="${isLastPage ? 'disabled' : ''}">下一页 &raquo;</a>`;
    
    html += '</div>';
    return html;
}

async function loadPushChannels() {
    try {
        const response = await fetch('/api/push_channels');
        if (!response.ok) throw new Error('获取推送渠道失败');
        const channels = await response.json();
        const pushTypeSelect = document.getElementById('push-type');
        pushTypeSelect.innerHTML = '';
        channels.forEach(channel => {
            const option = document.createElement('option');
            option.value = channel;
            option.textContent = channel;
            pushTypeSelect.appendChild(option);
        });
    } catch (error) {
        console.error(error);
    }
}

function openModal(ruleId = null) {
    const modal = document.getElementById('rule-modal');
    const form = document.getElementById('rule-form');
    form.reset();
    document.getElementById('rule-id').value = '';

    loadPushChannels().then(() => {
        if (ruleId) {
            const rule = currentRules.find(r => r.id === ruleId);
            if (rule) {
                document.getElementById('modal-title').innerText = '编辑规则';
                document.getElementById('rule-id').value = rule.id;
                document.getElementById('rule-name').value = rule.rule_name;
                document.getElementById('source-number').value = rule.source_number;
                document.getElementById('keywords').value = rule.keywords;
                document.getElementById('push-type').value = rule.push_type;
                document.getElementById('push-config').value = rule.push_config;
                document.getElementById('enabled').checked = rule.enabled;
                document.getElementById('is-default-forward').checked = rule.is_default_forward;
            }
        } else {
            document.getElementById('modal-title').innerText = '添加规则';
        }
        modal.style.display = 'block';
    });
}

function closeModal() {
    document.getElementById('rule-modal').style.display = 'none';
}

async function saveRule() {
    const ruleId = document.getElementById('rule-id').value;
    const rule = {
        rule_name: document.getElementById('rule-name').value,
        source_number: document.getElementById('source-number').value,
        keywords: document.getElementById('keywords').value,
        push_type: document.getElementById('push-type').value,
        push_config: document.getElementById('push-config').value,
        enabled: document.getElementById('enabled').checked,
        is_default_forward: document.getElementById('is-default-forward').checked
    };

    const isUpdate = !!ruleId;
    if (isUpdate) {
        rule.id = parseInt(ruleId, 10);
    }

    const url = isUpdate ? '/api/rules/update' : '/api/rules';
    const method = 'POST';

    try {
        const response = await fetch(url, {
            method: method,
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(rule)
        });

        if (response.ok) {
            closeModal();
            loadRules();
        } else {
            const errorText = await response.text();
            alert('保存规则失败: ' + errorText);
        }
    } catch (error) {
        console.error('保存规则时出错:', error);
        alert('保存规则时发生错误。');
    }
    return false;
}

async function deleteRule(ruleId) {
    if (!confirm('您确定要删除此规则吗？')) return;

    try {
        const response = await fetch('/api/rules/delete', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id: ruleId })
        });
        if (response.ok) {
            loadRules();
        } else {
            const errorText = await response.text();
            alert('删除规则失败: ' + errorText);
        }
    } catch (error) {
        console.error('删除规则时出错:', error);
        alert('删除规则时发生错误。');
    }
}

function loadLogs() { document.getElementById('content').innerHTML = '<h2>系统日志</h2><p>此功能待实现。</p>'; }
function loadStatus() { document.getElementById('content').innerHTML = '<h2>系统状态</h2><p>此功能待实现。</p>'; }

function loadDatabase() {
    const content = document.getElementById('content');
    let html = `
        <h2>数据库维护工具</h2>
        
        <div class="message warning" style="background-color: #fff3cd; color: #856404; border: 1px solid #ffeaa7; margin: 1rem 0; padding: 0.8rem; border-radius: 4px;">
            <strong>警告：</strong> 此工具允许直接执行SQL命令，请谨慎使用。为了安全，已禁用DROP、DELETE、TRUNCATE、ALTER等危险操作。
        </div>

        <div class="sql-editor-container" style="margin-bottom: 1rem;">
            <label for="sql-editor">SQL 命令：</label>
            <textarea id="sql-editor" class="sql-editor" placeholder="请输入SQL命令...\n\n示例：\nSELECT * FROM forward_rules LIMIT 10;\nSELECT COUNT(*) FROM sms_records;\nPRAGMA table_info(forward_rules);" style="width: 100%; min-height: 200px; padding: 1rem; border: 1px solid #ddd; border-radius: 4px; font-family: 'Courier New', Courier, monospace; font-size: 14px; resize: vertical; background-color: #f8f9fa; box-sizing: border-box;"></textarea>
        </div>

        <div class="button-group" style="margin: 1rem 0; display: flex; gap: 10px;">
            <button id="execute-btn" onclick="executeSQL()">执行 SQL</button>
            <button class="secondary" onclick="clearDatabaseEditor()" style="background-color: #6c757d;">清空</button>
            <button class="secondary" onclick="formatSQL()" style="background-color: #6c757d;">格式化</button>
        </div>

        <div id="db-loading" class="loading" style="display: none; color: #007bff;">执行中...</div>
        
        <div id="db-execution-info" class="execution-info" style="display: none; margin: 1rem 0; padding: 0.5rem; background-color: #e9ecef; border-radius: 4px; font-size: 0.9rem;">
            <span id="db-execution-time"></span> | 
            <span id="db-row-count"></span>
        </div>

        <div id="db-message-container"></div>

        <div id="db-results-container" class="results-container" style="display: none; margin-top: 1rem;">
            <h3>执行结果</h3>
            <div id="db-results-content"></div>
        </div>

        <div class="sql-examples" style="margin-top: 2rem; padding: 1rem; background-color: #f8f9fa; border-radius: 4px;">
            <h3 style="margin-top: 0;">常用 SQL 示例</h3>
            <p>点击下方示例可快速填入编辑器：</p>
            
            <div class="example-sql" onclick="insertDatabaseExample(this.textContent)" style="background-color: #e9ecef; padding: 0.5rem; margin: 0.5rem 0; border-radius: 3px; font-family: 'Courier New', Courier, monospace; font-size: 0.9rem; cursor: pointer;">
SELECT * FROM forward_rules LIMIT 10;
            </div>
            
            <div class="example-sql" onclick="insertDatabaseExample(this.textContent)" style="background-color: #e9ecef; padding: 0.5rem; margin: 0.5rem 0; border-radius: 3px; font-family: 'Courier New', Courier, monospace; font-size: 0.9rem; cursor: pointer;">
SELECT COUNT(*) as total_rules FROM forward_rules;
            </div>
            
            <div class="example-sql" onclick="insertDatabaseExample(this.textContent)" style="background-color: #e9ecef; padding: 0.5rem; margin: 0.5rem 0; border-radius: 3px; font-family: 'Courier New', Courier, monospace; font-size: 0.9rem; cursor: pointer;">
SELECT * FROM sms_records ORDER BY received_at DESC LIMIT 20;
            </div>
            
            <div class="example-sql" onclick="insertDatabaseExample(this.textContent)" style="background-color: #e9ecef; padding: 0.5rem; margin: 0.5rem 0; border-radius: 3px; font-family: 'Courier New', Courier, monospace; font-size: 0.9rem; cursor: pointer;">
SELECT COUNT(*) as total_sms FROM sms_records;
            </div>
            
            <div class="example-sql" onclick="insertDatabaseExample(this.textContent)" style="background-color: #e9ecef; padding: 0.5rem; margin: 0.5rem 0; border-radius: 3px; font-family: 'Courier New', Courier, monospace; font-size: 0.9rem; cursor: pointer;">
PRAGMA table_info(forward_rules);
            </div>
            
            <div class="example-sql" onclick="insertDatabaseExample(this.textContent)" style="background-color: #e9ecef; padding: 0.5rem; margin: 0.5rem 0; border-radius: 3px; font-family: 'Courier New', Courier, monospace; font-size: 0.9rem; cursor: pointer;">
PRAGMA table_info(sms_records);
            </div>
            
            <div class="example-sql" onclick="insertDatabaseExample(this.textContent)" style="background-color: #e9ecef; padding: 0.5rem; margin: 0.5rem 0; border-radius: 3px; font-family: 'Courier New', Courier, monospace; font-size: 0.9rem; cursor: pointer;">
SELECT name FROM sqlite_master WHERE type='table';
            </div>
        </div>
    `;
    content.innerHTML = html;
}

function executeSQL() {
    const sqlEditor = document.getElementById('sql-editor');
    const sqlCommand = sqlEditor.value.trim();
    
    if (!sqlCommand) {
        showDatabaseMessage('请输入SQL命令', 'error');
        return;
    }
    
    // 二次确认
    if (!confirm('确定要执行此SQL命令吗？\n\n' + sqlCommand)) {
        return;
    }
    
    const executeBtn = document.getElementById('execute-btn');
    const loading = document.getElementById('db-loading');
    
    executeBtn.disabled = true;
    loading.style.display = 'block';
    
    fetch('/api/database/execute', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ sql: sqlCommand })
    })
    .then(response => response.json())
    .then(data => {
        executeBtn.disabled = false;
        loading.style.display = 'none';
        
        if (data.success) {
            showDatabaseExecutionInfo(data.executionTime, data.rowCount || data.affectedRows);
            
            if (data.type === 'query' && data.data) {
                showDatabaseQueryResults(data.data);
                showDatabaseMessage('查询执行成功', 'success');
            } else {
                showDatabaseMessage(data.message || 'SQL执行成功', 'success');
                hideDatabaseResults();
            }
        } else {
            showDatabaseMessage('执行失败: ' + data.error, 'error');
            hideDatabaseResults();
        }
    })
    .catch(error => {
        executeBtn.disabled = false;
        loading.style.display = 'none';
        showDatabaseMessage('网络错误: ' + error.message, 'error');
        hideDatabaseResults();
    });
}

function showDatabaseMessage(message, type) {
    const container = document.getElementById('db-message-container');
    const messageClass = type === 'success' ? 'message success' : type === 'error' ? 'message error' : 'message';
    const bgColor = type === 'success' ? '#d4edda' : type === 'error' ? '#f8d7da' : '#fff3cd';
    const textColor = type === 'success' ? '#155724' : type === 'error' ? '#721c24' : '#856404';
    const borderColor = type === 'success' ? '#c3e6cb' : type === 'error' ? '#f5c6cb' : '#ffeaa7';
    
    container.innerHTML = `<div class="${messageClass}" style="background-color: ${bgColor}; color: ${textColor}; border: 1px solid ${borderColor}; margin: 1rem 0; padding: 0.8rem; border-radius: 4px;">${message}</div>`;
}

function showDatabaseExecutionInfo(executionTime, rowCount) {
    const infoDiv = document.getElementById('db-execution-info');
    const timeSpan = document.getElementById('db-execution-time');
    const countSpan = document.getElementById('db-row-count');
    
    timeSpan.textContent = `执行耗时: ${executionTime}ms`;
    countSpan.textContent = `影响行数: ${rowCount || 'N/A'}`;
    infoDiv.style.display = 'block';
}

function showDatabaseQueryResults(data) {
    const container = document.getElementById('db-results-container');
    const content = document.getElementById('db-results-content');
    
    if (!data || data.length === 0) {
        content.innerHTML = '<p>查询结果为空</p>';
        container.style.display = 'block';
        return;
    }
    
    // 构建表格
    const columns = Object.keys(data[0]);
    let html = '<table style="width: 100%; border-collapse: collapse; margin-top: 1rem;">';
    
    // 表头
    html += '<thead style="background-color: #e9ecef;"><tr>';
    columns.forEach(col => {
        html += `<th style="padding: 0.8rem; text-align: left; border-bottom: 1px solid #ddd;">${col}</th>`;
    });
    html += '</tr></thead>';
    
    // 表体
    html += '<tbody>';
    data.forEach(row => {
        html += '<tr>';
        columns.forEach(col => {
            const value = row[col] || '';
            html += `<td style="padding: 0.8rem; text-align: left; border-bottom: 1px solid #ddd; word-wrap: break-word; max-width: 200px;" title="${value}">${value}</td>`;
        });
        html += '</tr>';
    });
    html += '</tbody></table>';
    
    content.innerHTML = html;
    container.style.display = 'block';
}

function hideDatabaseResults() {
    document.getElementById('db-results-container').style.display = 'none';
}

function clearDatabaseEditor() {
    document.getElementById('sql-editor').value = '';
    document.getElementById('db-message-container').innerHTML = '';
    hideDatabaseResults();
    document.getElementById('db-execution-info').style.display = 'none';
}

function formatSQL() {
    const editor = document.getElementById('sql-editor');
    let sql = editor.value.trim();
    
    if (!sql) return;
    
    // 简单的SQL格式化
    sql = sql.replace(/\s+/g, ' ');
    sql = sql.replace(/;\s*/g, ';\n');
    sql = sql.replace(/\bSELECT\b/gi, 'SELECT');
    sql = sql.replace(/\bFROM\b/gi, '\nFROM');
    sql = sql.replace(/\bWHERE\b/gi, '\nWHERE');
    sql = sql.replace(/\bORDER BY\b/gi, '\nORDER BY');
    sql = sql.replace(/\bGROUP BY\b/gi, '\nGROUP BY');
    sql = sql.replace(/\bLIMIT\b/gi, '\nLIMIT');
    
    editor.value = sql;
}

function insertDatabaseExample(sql) {
    document.getElementById('sql-editor').value = sql.trim();
}

)rawliteral";

#endif // JS_CONTENT_H
