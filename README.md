# DX12 Backend

An ongoing **C++ DirectX 12 rendering backend** project designed for integration into a backend-agnostic engine architecture.

The current version focuses on initialising a DirectX 12 environment and validating how a future DX12 renderer can connect cleanly to the shared engine interface before expanding into full rendering systems.

> **Status:** Early development — DirectX 12 initialisation is implemented. Further rendering systems will be expanded after the shared engine interface has been fully tested.

---

## 🧠 Overview

DX12 Backend is being developed as a modern rendering backend for a modular C++ engine architecture.

The project explores how low-level graphics systems can be separated from gameplay and engine logic through interfaces and abstraction layers. This allows engine systems to remain independent from the graphics API while supporting future backend flexibility and scalability.

---

## ⚙️ Current Features

- DirectX 12 initialisation
- Backend abstraction architecture
- Shared engine interface integration
- Modular backend structure
- Visual Studio project setup
- Foundation for future renderer expansion

---

## 🧱 Architecture

The backend separates DirectX 12-specific implementation details from shared engine systems using interfaces and abstraction layers.

Gameplay and engine logic communicate through the shared `EngineInterface` submodule, while the DX12 backend manages graphics API setup and future rendering responsibilities.

```text
DX12Backend/
├── EngineInterface/     → Shared engine abstraction layer submodule
├── DX12BackEnd/         → DirectX 12 backend implementation
├── Resources/           → Future shaders and rendering assets
├── *.sln                → Visual Studio solution and project files
└── .gitmodules          → Submodule configuration
```

This structure is intended to support future rendering backends while minimising duplication between engine systems and graphics implementations.

---

## 🛠 Technologies

- C++
- DirectX 12
- Visual Studio
- Windows SDK
- Backend abstraction
- Low-level graphics programming

---

## 🚀 Getting Started

### Clone with Submodules

```bash
git clone <repository-url>
cd DX12Backend
git submodule update --init --recursive
```

### Requirements

- Visual Studio 2019 or 2022
- Windows 10/11 SDK
- DirectX 12-compatible GPU and drivers

### Build

1. Open the Visual Studio solution.
2. Ensure the DirectX 12 development environment is configured.
3. Select **Debug** or **Release**.
4. Build and run the project.

---

## 🔭 Next Steps

- Fully test the shared engine interface before expanding the backend
- Validate backend creation through the engine factory/interface layer
- Add swap chain setup and presentation flow
- Implement command queue, allocator, and command list systems
- Add descriptor heap and GPU resource handling
- Introduce basic clear-screen rendering
- Add shader compilation and pipeline state configuration
- Connect rendering operations to engine-level drawing interfaces
- Expand toward reusable renderer systems once the abstraction layer is stable

---

## Future Work

- Multi-backend rendering support
- Shader management systems
- ECS integration
- Advanced rendering techniques
- Debug tooling and graphics diagnostics
- Performance and scalability improvements

---

## Project Context

This project was developed as part of a games programming portfolio to explore DirectX 12 initialisation, backend abstraction, engine integration, and scalable rendering architecture.
