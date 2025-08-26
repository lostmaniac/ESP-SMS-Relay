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

)rawliteral";

#endif // JS_CONTENT_H
