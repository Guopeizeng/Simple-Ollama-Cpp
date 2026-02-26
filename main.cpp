#include <iostream>
#include <string>
#include <vector>
#include "httplib.h"
#include "json.hpp"
#include <cstdlib>
#include <memory>
#include <fstream>

using namespace std;
using json = nlohmann::json;

// ==========================================
// 核心类：AIClient
// ==========================================
class AIClient {
private:
    string m_host;
    int m_port;
    string m_model;
    std::unique_ptr<httplib::Client> m_cli;
    std::ofstream my_logFile;
    string m_context_buffer;
    const size_t MAX_CONTEXT_LENGTH=2000;

public:
    AIClient(string host, int port, string model) 
        : m_host(host), m_port(port), m_model(model) 
    {
        m_cli=std::make_unique<httplib::Client>(host, port);
        my_logFile.open("chat_history.txt",std::ios::app);
        m_cli->set_connection_timeout(300, 0); // 300秒
        m_cli->set_read_timeout(300, 0);       // 300秒

    }

    string chat(const string& user_input) {

        string full_prompt=m_context_buffer+"\n[User]:"+user_input+"[AI]:";

        json request_body = {
            {"model", m_model},
            {"prompt", full_prompt}, 
            {"stream", false}       
        };

        my_logFile<<"[用户]:"<<user_input<<endl;
        // 发送请求
        auto res = m_cli->Post("/api/generate", request_body.dump(), "application/json");

        string reply="";

        // --- 详细的错误诊断逻辑 ---
        if (res) {
            if (res->status == 200) {
                try {
                    json response = json::parse(res->body);
                    reply= response["response"]; 
                    my_logFile<<"[AI]:"<<reply<<endl;
                    my_logFile.flush();
                } catch (json::parse_error& e) {
                    return "[Error] JSON 解析失败: " + string(e.what());
                }
            } else {
                return "[Error] 服务器连上了，但拒绝服务。状态码: " + to_string(res->status);
            }
        } else {
            // 打印具体的连接错误原因
            auto err = res.error();
            string err_msg;
            if (err == httplib::Error::Connection) err_msg = "Connection (连接被拒/端口被墙)";
            else if (err == httplib::Error::BindIPAddress) err_msg = "BindIPAddress (IP绑定失败)";
            else if (err == httplib::Error::Read) err_msg = "Read (读取超时)";
            else if (err == httplib::Error::Write) err_msg = "Write (发送失败)";
            else if (err == httplib::Error::ExceedRedirectCount) err_msg = "ExceedRedirectCount";
            else if (err == httplib::Error::Canceled) err_msg = "Canceled";
            else err_msg = "Unknown Error (可能是代理拦截)";

            return "[Fatal Error] 请求根本没发出去！原因: " + err_msg;
        }

        //成功获得回复后追加添加进入历史记录
        string new_round = "\n[User]:"+user_input+"\n[AI]:"+reply;
        m_context_buffer+=new_round;

        enforce_limit();
        
        return reply;
    }
private:
void enforce_limit() {
    if (m_context_buffer.length() > MAX_CONTEXT_LENGTH) {
        size_t start_pos = m_context_buffer.length() - MAX_CONTEXT_LENGTH;

        // ✅ 修复：对齐到 UTF-8 字符边界
        // UTF-8 延续字节的特征：最高两位为 10（即 0x80~0xBF）
        // 只要当前字节是延续字节，就往后移一位，直到找到字符起始字节
        while (start_pos < m_context_buffer.length() &&
               (static_cast<unsigned char>(m_context_buffer[start_pos]) & 0xC0) == 0x80) {
            start_pos++;
        }

        m_context_buffer = m_context_buffer.substr(start_pos);

        size_t first_newline = m_context_buffer.find('\n');
        if (first_newline != string::npos) {
            m_context_buffer = m_context_buffer.substr(first_newline + 1);
        }
    }
}
};

// ==========================================
// 主程序
// ==========================================
int main() {
    system("chcp 65001"); 

    putenv("HTTP_PROXY=");
    putenv("HTTPS_PROXY=");
    putenv("ALL_PROXY=");

    cout << "=== 本地 AI 助手 (诊断版) ===" << endl;
    cout << "正在连接 127.0.0.1:11434..." << endl;

    // 务必确保这里的名字和你 ollama list 看到的一模一样
    AIClient bot("127.0.0.1", 11434, "qwen2.5-coder:7b"); 

    cout << ">>> 客户端就绪。尝试发送第一条消息..." << endl;

    // 自动发送一条测试消息
    string reply = bot.chat("你好");
    cout << "[AI 回复]: " << reply << endl;

    if (reply.find("[Fatal Error]") != string::npos) {
        cout << "\n=== 诊断建议 ===" << endl;
        cout << "1. 请彻底关闭所有 VPN / 加速器软件。" << endl;
        cout << "2. 请检查 Windows 防火墙是否拦截了 main.exe。" << endl;
    }

    // 进入手动聊天
    string input;
    while (true) {
        cout << "\n[你]: ";
        if (!getline(cin, input)) break;
        if (input == "exit") break;
        if (input.empty()) continue;

        string reply = bot.chat(input);
        cout << "[AI]: " << reply << endl;
    }

    return 0;
}

