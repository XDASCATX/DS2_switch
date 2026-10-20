# DARK SOULS II: SCHOLAR OF THE FIRST SIN
### A native Nintendo Switch port in the making.

<p align="center">
  <strong>BEARER OF THE CURSE.</strong><br>
  <em>One world. One console. No shortcuts.</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/PLATFORM-Nintendo%20Switch-8b0000?style=for-the-badge&logo=nintendo-switch&logoColor=white" alt="Nintendo Switch">
  <img src="https://img.shields.io/badge/ENGINEERING-Static%20Recompilation-242424?style=for-the-badge" alt="Static recompilation">
  <img src="https://img.shields.io/badge/STATUS-IN%20DEVELOPMENT-b8860b?style=for-the-badge" alt="In development">
</p>

---

> *In Drangleic, even the fire fades.*
>
> This project aims to bring **Dark Souls II: Scholar of the First Sin** — the Xbox 360 release — to the Nintendo Switch through static recompilation and a native Switch application.

Not a streaming solution. Not a traditional emulator frontend.

**The goal is to turn the original game's executable code into a program that can run natively on Switch hardware.**

The journey has only begun.

---

## 🗡️ THE QUEST

Dark Souls II is a game about persistence. Porting it to a different hardware architecture is, fittingly, a test of the same quality.

This project investigates a native port of the Xbox 360 version, using a static recompilation approach and a Switch-targeted build pipeline.

The objective is ambitious:

- **Native execution** — recompile the original game's code for the target platform.
- **Real hardware** — work toward running the game on Nintendo Switch itself.
- **30 FPS target** — aim for a stable, consistent gameplay experience.
- **Preservation** — explore another way to experience a classic on modern hardware.

These are development goals, not claims of completed functionality.

## 🔥 DEVELOPMENT STATUS

**The project is under active development.**

The Switch-targeted build pipeline has reached C++ compilation of generated game-code translation units. The current build has progressed through hundreds of compilation tasks, but a successful final link and a boot on actual Switch hardware have not yet been confirmed.

| Milestone | Status |
|---|---|
| Project and build pipeline | In progress |
| Generated C++ compilation | In progress |
| Successful final application build | Not yet confirmed |
| First boot on Nintendo Switch | Not yet confirmed |
| Main menu | Not yet confirmed |
| Gameplay | Not yet confirmed |
| Stable 30 FPS | Target, not yet verified |

The priority is simple: **working code first, measurable progress second, promises last.**

## ⚙️ UNDER THE HOOD

The source target is the Xbox 360 release of *Dark Souls II: Scholar of the First Sin*.

| Property | Target |
|---|---|
| Original platform | Xbox 360 |
| Game | Dark Souls II: Scholar of the First Sin |
| Title ID | `465307E4` |
| Game version | `1.10` |
| Calibration version | `1.15` |
| Destination | Nintendo Switch |
| Approach | Static recompilation |
| Performance objective | 30 FPS |

### The pipeline

```text
Original Xbox 360 executable
            │
            ▼
  Static recompilation
            │
            ▼
     Generated C++ code
            │
            ▼
   Switch-targeted compilation
            │
            ▼
     Native application
            │
            ▼
   Hardware validation
            │
            ▼
      Drangleic awaits
```

Each stage must be validated. Successful compilation alone does not guarantee a bootable application, correct rendering, working audio, or playable gameplay.

## 🧭 THE ROAD AHEAD

The work is organized around engineering milestones rather than arbitrary feature promises.

- [ ] Complete the current Switch-targeted build.
- [ ] Resolve compilation and linking failures, if any.
- [ ] Identify and validate the resulting application artifact.
- [ ] Achieve the first confirmed boot on Switch hardware.
- [ ] Investigate platform-specific runtime requirements.
- [ ] Bring up rendering, input, audio, and game progression.
- [ ] Profile performance and work toward the 30 FPS target.
- [ ] Document reproducible builds and verified results.

Milestones will be checked off when they are demonstrated, not merely when they sound plausible.

## 🛡️ PROJECT PRINCIPLES

**Accuracy over hype.** Development status reflects verified results. Untested features remain untested.

**Original game data stays yours.** This repository does not distribute the commercial game, its executable, assets, or other copyrighted game content.

**No DRM circumvention.** The project does not aim to bypass DRM or defeat copy protection.

**Preserve the source executable.** The original `Default.xex` is treated as source input, not as a file to modify.

**Controlled engineering changes.** SDK and toolchain changes require deliberate review; undocumented compatibility assumptions are not treated as facts.

**Reproducibility matters.** Build instructions, diagnostics, and test results should make progress independently verifiable.

## 📦 REQUIREMENTS & AVAILABILITY

This is an engineering project, not a ready-to-install game release.

- A legally obtained copy of the Xbox 360 edition is the intended source.
- A compatible development environment and Switch-targeted toolchain are required to build the project.
- The precise build prerequisites and extraction workflow will be documented as they are verified.

**There is currently no verified playable release advertised by this README.** A completed compilation task is not the same as a playable port.

## 🤝 CONTRIBUTING

Research, technical investigation, reproducible bug reports, and carefully documented fixes are valuable.

Useful contributions should include:

- The exact environment and toolchain used.
- The command or procedure that reproduces the result.
- Relevant build output or diagnostic logs.
- A clear distinction between observed behavior and assumptions.

Please avoid submitting copyrighted game files, undocumented binary modifications, or unverified claims of compatibility.

## ⚖️ DISCLAIMER

Dark Souls II: Scholar of the First Sin is a property of its respective rights holders, including FromSoftware and Bandai Namco Entertainment. Nintendo, Xbox, and Microsoft are trademarks of their respective owners.

This is an independent, unofficial fan engineering project. It is not affiliated with, endorsed by, or sponsored by FromSoftware, Bandai Namco Entertainment, Nintendo, or Microsoft.

No original game content is distributed by this repository.

---

<p align="center">
  <strong>SEEK THE KING.</strong><br>
  <em>And may the flames guide thee.</em>
</p>
