# 🌐 HTTP Web Proxy Server in C++

This project is a basic **HTTP web proxy server** implemented in C++ using low-level socket programming. It was designed to forward HTTP requests from clients to destination servers and relay responses back to the clients — simulating real-world proxy behavior.

The goal of this project was to apply foundational knowledge of **socket programming** in a practical context and gain a **hands-on understanding of the HTTP protocol** and request/response lifecycle.

---

## 📦 What It Does

- Accepts HTTP requests from client browsers (e.g., GET)
- Forwards the request to the appropriate destination (end) server
- Receives the response from the server
- Relays the response back to the original client
- Maintains a basic event loop for handling multiple requests

---

## 🛠️ Tech Stack

- **Language:** C++  
- **Networking:** POSIX sockets (TCP)  
- **Protocols:** HTTP/1.0 and HTTP/1.1 (basic support)  
- **Operating System:** Linux/Unix-based environments

---

## 🧠 Key Learning Objectives

- Deepened understanding of the HTTP protocol and how web traffic flows  
- Gained experience with socket-level networking and message forwarding  
- Implemented request parsing, host resolution, and stream relaying manually  
- Explored practical use cases for proxies (e.g., caching, filtering, security)
