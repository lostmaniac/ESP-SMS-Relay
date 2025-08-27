#ifndef CSS_CONTENT_H
#define CSS_CONTENT_H

const char CSS_CONTENT[] PROGMEM = R"rawliteral(
body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; margin: 0; background-color: #f4f4f9; color: #333; }
nav { background-color: #4a4a4a; color: white; padding: 1rem; }
.nav-container { max-width: 1200px; margin: auto; display: flex; justify-content: space-between; align-items: center; }
.nav-links a { color: white; text-decoration: none; margin-left: 1.5rem; font-size: 1.1rem; }
main { max-width: 1200px; margin: 1rem auto; padding: 1rem; background-color: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
table { width: 100%; border-collapse: collapse; margin-top: 1rem; }
th, td { padding: 0.8rem; text-align: left; border-bottom: 1px solid #ddd; }
thead { background-color: #e9ecef; }
button { background-color: #007bff; color: white; border: none; padding: 0.6rem 1.2rem; border-radius: 5px; cursor: pointer; font-size: 1rem; }
button:hover { background-color: #0056b3; }
button.secondary:hover { background-color: #5a6268; }
button:disabled { background-color: #6c757d; cursor: not-allowed; }
.example-sql:hover { background-color: #dee2e6; }
.action-buttons button { margin-right: 5px; }
.modal { display: none; position: fixed; z-index: 1; left: 0; top: 0; width: 100%; height: 100%; overflow: auto; background-color: rgba(0,0,0,0.4); }
.modal-content { background-color: #fefefe; margin: 10% auto; padding: 20px; border: 1px solid #888; width: 80%; max-width: 600px; border-radius: 8px; }
.close-button { color: #aaa; float: right; font-size: 28px; font-weight: bold; cursor: pointer; }
form label { display: block; margin-top: 1rem; }
form input[type=text], form select, form textarea { width: 100%; padding: 8px; margin-top: 5px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
form button { margin-top: 1.5rem; }
.pagination { display: flex; justify-content: center; margin-top: 1.5rem; }
.pagination a { color: #007bff; padding: 8px 16px; text-decoration: none; transition: background-color .3s; border: 1px solid #ddd; margin: 0 4px; border-radius: 4px; }
.pagination a.active { background-color: #007bff; color: white; border: 1px solid #007bff; }
.pagination a:hover:not(.active) { background-color: #ddd; }
.pagination a.disabled { color: #ccc; cursor: not-allowed; pointer-events: none; }
.sms-content { max-width: 400px; word-wrap: break-word; }
.docs-container { background-color: #fff; padding: 1rem; border-radius: 8px; margin-top: 1rem; }
.docs-container pre { white-space: pre-wrap; word-wrap: break-word; font-family: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, Courier, monospace; font-size: 0.9rem; color: #333; }
.form-group { margin-bottom: 1rem; }
.form-group label { display: block; margin-bottom: 0.5rem; font-weight: bold; }
.form-group input[type="text"], .form-group input[type="password"] { width: 100%; padding: 0.8rem; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; font-size: 1rem; }
.form-group button { margin-top: 1.5rem; width: auto; }
.message { margin-top: 1rem; padding: 0.8rem; border-radius: 4px; }
.message.success { background-color: #d4edda; color: #155724; border-color: #c3e6cb; }
.message.error { background-color: #f8d7da; color: #721c24; border-color: #f5c6cb; }
)rawliteral";

#endif // CSS_CONTENT_H
