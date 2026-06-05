#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

inline std::string get_content_type(const std::string& path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".css"))  return "text/css";
    if (path.ends_with(".js"))   return "application/javascript";
    if (path.ends_with(".png"))  return "image/png";
    if (path.ends_with(".jpg"))  return "image/jpeg";
    return "text/plain";
}

inline std::string read_proc_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) return "unavailable";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

inline std::string get_stats() {
    std::string uptime  = read_proc_file("/proc/uptime");
    std::string loadavg = read_proc_file("/proc/loadavg");
    std::string meminfo = read_proc_file("/proc/meminfo");

    std::string mem_total, mem_available;
    std::istringstream mem_stream(meminfo);
    std::string line;
    while (std::getline(mem_stream, line)) {
        if (line.starts_with("MemTotal"))     mem_total     = line;
        if (line.starts_with("MemAvailable")) mem_available = line;
    }

    std::ostringstream html;
    html << "<!DOCTYPE html><html><head><title>Server Stats</title></head><body>";
    html << "<h1>Server Stats</h1>";
    html << "<h3>Uptime</h3><pre>" << uptime << "</pre>";
    html << "<h3>Load Average</h3><pre>" << loadavg << "</pre>";
    html << "<h3>Memory</h3><pre>" << mem_total << "\n" << mem_available << "</pre>";
    html << "</body></html>";
    return html.str();
}

inline void send_response(int client_fd, const std::string& status, const std::string& content_type, const std::string& body) {
    std::string response =
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + body;
    write(client_fd, response.c_str(), response.size());
}

inline void serve_file(int client_fd, const std::string& filepath) {
    int file_fd = open(filepath.c_str(), O_RDONLY);

    if (file_fd < 0) {
        std::string body = "<html><body><h1>404 Not Found</h1></body></html>";
        send_response(client_fd, "404 Not Found", "text/html", body);
        return;
    }

    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;

    std::string content_type = get_content_type(filepath);
    std::string headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Content-Length: " + std::to_string(file_size) + "\r\n"
        "Connection: close\r\n"
        "\r\n";

    write(client_fd, headers.c_str(), headers.size());
    sendfile(client_fd, file_fd, nullptr, file_size);
    close(file_fd);
}

inline void handle_client(int client_fd) {
    char buffer[4096] = {};
    ssize_t bytes = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes <= 0) {
        close(client_fd);
        return;
    }

    std::string request(buffer);
    std::string path = "/";

    auto start = request.find("GET ");
    if (start != std::string::npos) {
        start += 4;
        auto end = request.find(" ", start);
        if (end != std::string::npos)
            path = request.substr(start, end - start);
    }

    if (path == "/stats") {
        send_response(client_fd, "200 OK", "text/html", get_stats());
    } else {
        if (path == "/") path = "/index.html";
        serve_file(client_fd, "www" + path);
    }

    close(client_fd);
}