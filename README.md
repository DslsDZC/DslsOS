# DslsOS Documentation

English / [简体中文](README_CN.md) / [日本語](README_JP.md) / [Français](README_FR.md) / [Deutsch](README_DE.md)

![Static Badge](https://img.shields.io/badge/License_GPLv3-0?logo=gnu&color=8A2BE2)
## Overview

DslsOS is a modern operating system featuring a microkernel architecture with advanced distributed computing capabilities. Designed to provide efficient, secure, and flexible runtime environments for diverse computing scenarios, it combines the stability of microkernel design with the power of distributed systems.

## Core Features

### 🏗️ Microkernel Design
- Minimal privileged components in kernel space
- Core services (process management, memory allocation, IPC) implemented as modular components
- Enhanced system stability and reduced attack surface

### 🌐 Distributed Computing Support
- Native cluster management capabilities
- Automatic node coordination and service deployment
- Support for embedded devices to large-scale distributed systems

### ⚡ Intelligent Scheduling
- Dynamic task scheduling based on system load
- Multiple scheduling algorithms for optimal resource utilization
- Real-time performance adaptation

### 🔒 Security Architecture
- Zero-trust security model
- Integrated authentication, authorization, and encryption
- Comprehensive audit logging and monitoring

## Distributed Computing Implementation

### Core Architecture

**Distributed Management Module** (`distributed_management.c`):
- `NODE_INFO`, `CLUSTER_INFO`, `SERVICE_INFO` structures for system modeling
- `DmCreateCluster`, `DmJoinCluster`, `DmCreateService` APIs for cluster lifecycle management

### Key Mechanisms

#### 1. Node & Cluster Management
- Automatic node discovery via service discovery mechanism
- Heartbeat monitoring for node health tracking
- Support for specialized node types (compute, storage, etc.)

#### 2. Service Deployment
- Container-based service deployment using lightweight virtualization
- Dynamic service scaling with `DmScaleService` interface
- Dependency-aware service orchestration

#### 3. Load Balancing
- Multiple algorithms: Round Robin, Least Connections, IP Hash
- Real-time resource-aware scheduling
- Automatic task migration based on node load

#### 4. High Availability
- Automatic fault detection and recovery
- Service failover to backup nodes
- Self-healing cluster capabilities

## System Architecture

### Layered Design
- **Core Layer**: Microkernel handling process, memory, IPC
- **Service Layer**: Modular components (distributed management, containers, security)
- **Hardware Layer**: Hardware abstraction for cross-platform compatibility

### Key Components

**File System** (`dslsfs.c`):
- Distributed storage across multiple nodes
- Automatic data replication and caching
- Transactional operations with journaling

**Container System** (`container_system.c`):
- Lightweight virtualization with process isolation
- Resource limits and monitoring
- Container networking and storage

**Security System** (`security_architecture.c`):
- Multi-factor authentication
- Role-based access control
- End-to-end encryption

## Development Ecosystem

### Build & Tools
- Cross-platform building with CMake
- Support for GCC, Clang compilers
- Debug and Release compilation modes

### Coding Standards
- Unified coding conventions
- Doxygen-compatible documentation
- Structured contribution process

## Target Audience

- **Kernel Developers**: Core module design and optimization
- **Application Developers**: Software development using system APIs
- **System Administrators**: Deployment environment management
- **Security Researchers**: System security analysis

## Open Source License

Released under GNU General Public License v3 (GPLv3), ensuring freedom to use, modify, and distribute the software.

---

## ⭐ Contributing

1. Fork this repository
2. Create your branch (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 🌟 Acknowledgments

Gratitude to all developers who contributed code, documentation, and suggestions to this project!

<p align="center">
  <a href="https://github.com/DslsDZC/DslsOS/graphs/contributors">
    <img src="https://contrib.rocks/image?repo=DslsDZC/DslsOS" alt="Contributors">
  </a>
</p>

---

## Star

[![Star History Chart](https://api.star-history.com/svg?repos=DslsDZC/DslsOS&type=Date)](https://star-history.com/#DslsDZC/DslsOS&Date)

---

*Whether building embedded systems, deploying distributed clusters, or developing customized applications, DslsOS provides reliable technical foundations and flexible expansion capabilities.*
