#!/usr/bin/env python3
"""Minimal CoopAndreas launcher focused on first-time usability."""

from __future__ import annotations

import json
import socket
import subprocess
import threading
from dataclasses import asdict, dataclass
from pathlib import Path
import tkinter as tk
from tkinter import filedialog, messagebox, ttk


CONFIG_PATH = Path(__file__).resolve().with_name("launcher_config.json")
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 6767


@dataclass
class LauncherConfig:
    nickname: str = "Player"
    host: str = DEFAULT_HOST
    port: int = DEFAULT_PORT
    gta_path: str = ""


class SetupWizard:
    """First-run setup wizard shown when config is missing or incomplete."""

    def __init__(self, app: "LauncherApp") -> None:
        self.app = app
        self.root = app.root
        self.window = tk.Toplevel(self.root)
        self.window.title("CoopAndreas First-Run Setup")
        self.window.geometry("760x520")
        self.window.transient(self.root)
        self.window.grab_set()

        self.nickname_var = tk.StringVar(value=app.nickname_var.get())
        self.host_var = tk.StringVar(value=app.host_var.get() or DEFAULT_HOST)
        self.port_var = tk.StringVar(value=app.port_var.get() or str(DEFAULT_PORT))
        self.gta_path_var = tk.StringVar(value=app.gta_path_var.get())

        self.page_status_vars = [tk.StringVar() for _ in range(4)]
        self.summary_var = tk.StringVar(value="")

        self.notebook: ttk.Notebook
        self.back_btn: ttk.Button
        self.next_btn: ttk.Button
        self.finish_btn: ttk.Button

        self._build_ui()
        self._refresh_step_state()
        self.window.protocol("WM_DELETE_WINDOW", self._close)

    def _build_ui(self) -> None:
        wrapper = ttk.Frame(self.window, padding=12)
        wrapper.pack(fill=tk.BOTH, expand=True)

        self.notebook = ttk.Notebook(wrapper)
        self.notebook.pack(fill=tk.BOTH, expand=True)
        self.notebook.bind("<<NotebookTabChanged>>", lambda _evt: self._refresh_step_state())

        self._build_welcome_page()
        self._build_path_page()
        self._build_validation_page()
        self._build_connection_page()
        self._build_summary_page()

        nav = ttk.Frame(wrapper, padding=(0, 12, 0, 0))
        nav.pack(fill=tk.X)
        self.back_btn = ttk.Button(nav, text="Back", command=self._go_back)
        self.back_btn.pack(side=tk.LEFT)

        self.next_btn = ttk.Button(nav, text="Next", command=self._go_next)
        self.next_btn.pack(side=tk.RIGHT)

        self.finish_btn = ttk.Button(nav, text="Save and continue", command=self._finish)
        self.finish_btn.pack(side=tk.RIGHT, padx=(0, 8))

    def _build_welcome_page(self) -> None:
        page = ttk.Frame(self.notebook, padding=14)
        self.notebook.add(page, text="1) Welcome")
        ttk.Label(page, text="Welcome to the CoopAndreas launcher", font=("TkDefaultFont", 13, "bold")).pack(anchor="w")
        ttk.Label(
            page,
            text=(
                "This wizard will guide you through selecting GTA:SA, validating required files, "
                "and entering your multiplayer connection settings."
            ),
            wraplength=680,
            justify="left",
        ).pack(anchor="w", pady=(8, 10))

        ttk.Label(page, text="What this launcher does:", font=("TkDefaultFont", 10, "bold")).pack(anchor="w")
        bullets = ttk.Label(
            page,
            text=(
                "• Starts/stops the local CoopAndreas server\n"
                "• Launches GTA:SA after file validation\n"
                "• Saves host, port, nickname and GTA folder"
            ),
            justify="left",
        )
        bullets.pack(anchor="w", pady=(4, 10))

        ttk.Label(page, textvariable=self.page_status_vars[0], foreground="#1a7f37").pack(anchor="w", pady=(8, 0))

    def _build_path_page(self) -> None:
        page = ttk.Frame(self.notebook, padding=14)
        self.notebook.add(page, text="2) GTA folder")

        ttk.Label(page, text="Select your GTA:SA installation folder", font=("TkDefaultFont", 11, "bold")).pack(anchor="w")

        row = ttk.Frame(page)
        row.pack(fill=tk.X, pady=(12, 6))
        ttk.Entry(row, textvariable=self.gta_path_var).pack(side=tk.LEFT, fill=tk.X, expand=True)
        ttk.Button(row, text="Browse", command=self._browse_gta_path).pack(side=tk.LEFT, padx=(8, 0))

        ttk.Label(
            page,
            text="Expected to contain gta_sa.exe and CoopAndreas files.",
            foreground="#555555",
        ).pack(anchor="w")

        ttk.Label(page, textvariable=self.page_status_vars[1], wraplength=680).pack(anchor="w", pady=(12, 0))

    def _build_validation_page(self) -> None:
        page = ttk.Frame(self.notebook, padding=14)
        self.notebook.add(page, text="3) Validate files")

        ttk.Label(page, text="Required files check", font=("TkDefaultFont", 11, "bold")).pack(anchor="w")
        ttk.Label(
            page,
            text=(
                "Required: gta_sa.exe, CoopAndreasSA.dll, eax.dll, "
                "and one of CoopAndreas/main.scm or CoopAndreas/script.img"
            ),
            wraplength=680,
            justify="left",
        ).pack(anchor="w", pady=(8, 8))

        ttk.Button(page, text="Re-check now", command=self._refresh_step_state).pack(anchor="w", pady=(0, 8))
        ttk.Label(page, textvariable=self.page_status_vars[2], wraplength=680, justify="left").pack(anchor="w")

    def _build_connection_page(self) -> None:
        page = ttk.Frame(self.notebook, padding=14)
        self.notebook.add(page, text="4) Connection")

        ttk.Label(page, text="Connection settings", font=("TkDefaultFont", 11, "bold")).grid(row=0, column=0, columnspan=2, sticky="w")

        ttk.Label(page, text="Nickname").grid(row=1, column=0, sticky="w", pady=(12, 0))
        ttk.Entry(page, textvariable=self.nickname_var, width=30).grid(row=1, column=1, sticky="we", padx=(8, 0), pady=(12, 0))

        ttk.Label(page, text="Host").grid(row=2, column=0, sticky="w", pady=(8, 0))
        ttk.Entry(page, textvariable=self.host_var, width=30).grid(row=2, column=1, sticky="we", padx=(8, 0), pady=(8, 0))

        ttk.Label(page, text="Port").grid(row=3, column=0, sticky="w", pady=(8, 0))
        ttk.Entry(page, textvariable=self.port_var, width=12).grid(row=3, column=1, sticky="w", padx=(8, 0), pady=(8, 0))

        ttk.Label(page, textvariable=self.page_status_vars[3], wraplength=680).grid(row=4, column=0, columnspan=2, sticky="w", pady=(12, 0))
        page.columnconfigure(1, weight=1)

    def _build_summary_page(self) -> None:
        page = ttk.Frame(self.notebook, padding=14)
        self.notebook.add(page, text="5) Summary")
        ttk.Label(page, text="Summary", font=("TkDefaultFont", 11, "bold")).pack(anchor="w")
        ttk.Label(page, text="Review and click Save and continue.", foreground="#555555").pack(anchor="w", pady=(2, 8))
        ttk.Label(page, textvariable=self.summary_var, justify="left", wraplength=680).pack(anchor="w")

    def _close(self) -> None:
        self.window.grab_release()
        self.window.destroy()

    def _browse_gta_path(self) -> None:
        selected = filedialog.askdirectory(title="Select GTA:SA directory")
        if selected:
            self.gta_path_var.set(selected)
            self._refresh_step_state()

    def _current_index(self) -> int:
        return self.notebook.index(self.notebook.select())

    def _go_back(self) -> None:
        idx = self._current_index()
        if idx > 0:
            self.notebook.select(idx - 1)
            self._refresh_step_state()

    def _go_next(self) -> None:
        idx = self._current_index()
        if not self._can_advance_from(idx):
            self._refresh_step_state()
            return

        self._persist_current_step(idx)
        next_idx = min(idx + 1, len(self.notebook.tabs()) - 1)
        self.notebook.select(next_idx)
        self._refresh_step_state()

    def _set_status(self, step_index: int, text: str, ok: bool) -> None:
        prefix = "✅" if ok else "⚠️"
        self.page_status_vars[step_index].set(f"{prefix} {text}")

    def _refresh_step_state(self) -> None:
        self._set_status(0, "Ready to begin setup.", True)

        path_text = self.gta_path_var.get().strip()
        if path_text and Path(path_text).exists():
            self._set_status(1, f"Folder selected: {path_text}", True)
        elif path_text:
            self._set_status(1, f"Folder does not exist: {path_text}", False)
        else:
            self._set_status(1, "Choose your GTA:SA folder to continue.", False)

        game_result = self.app.validate_game_install(self.gta_path_var.get().strip())
        if game_result.ok:
            self._set_status(2, "All required game files were found.", True)
        else:
            missing_lines = "\n".join(f"- {item}" for item in game_result.missing)
            details = missing_lines or "Set GTA folder first."
            self._set_status(2, f"Missing required files:\n{details}", False)

        conn_result = self.app.validate_connection_settings(
            self.nickname_var.get(),
            self.host_var.get(),
            self.port_var.get(),
        )
        if conn_result.ok:
            self._set_status(3, "Connection values look good.", True)
        else:
            self._set_status(3, conn_result.error or "Invalid connection settings.", False)

        self.summary_var.set(
            "\n".join(
                [
                    f"Nickname: {self.nickname_var.get().strip() or '(empty)'}",
                    f"Host: {self.host_var.get().strip() or '(empty)'}",
                    f"Port: {self.port_var.get().strip() or '(empty)'}",
                    f"GTA path: {self.gta_path_var.get().strip() or '(empty)'}",
                    "",
                    self.page_status_vars[2].get(),
                    self.page_status_vars[3].get(),
                ]
            )
        )

        idx = self._current_index()
        last_idx = len(self.notebook.tabs()) - 1
        self.back_btn.configure(state=tk.NORMAL if idx > 0 else tk.DISABLED)
        self.next_btn.configure(state=tk.NORMAL if idx < last_idx else tk.DISABLED)
        can_finish = self.app.validate_game_install(self.gta_path_var.get().strip()).ok and conn_result.ok
        self.finish_btn.configure(state=tk.NORMAL if idx == last_idx and can_finish else tk.DISABLED)

    def _can_advance_from(self, idx: int) -> bool:
        if idx == 1:
            return Path(self.gta_path_var.get().strip()).exists()
        if idx == 2:
            return self.app.validate_game_install(self.gta_path_var.get().strip()).ok
        if idx == 3:
            return self.app.validate_connection_settings(
                self.nickname_var.get(), self.host_var.get(), self.port_var.get()
            ).ok
        return True

    def _persist_current_step(self, idx: int) -> None:
        if idx < 1:
            return
        self.app.apply_config_values(
            nickname=self.nickname_var.get().strip() or "Player",
            host=self.host_var.get().strip() or DEFAULT_HOST,
            port_text=self.port_var.get().strip() or str(DEFAULT_PORT),
            gta_path=self.gta_path_var.get().strip(),
        )
        self.app.save_current_config()

    def _finish(self) -> None:
        if not self.app.validate_game_install(self.gta_path_var.get().strip()).ok:
            self._refresh_step_state()
            return
        if not self.app.validate_connection_settings(self.nickname_var.get(), self.host_var.get(), self.port_var.get()).ok:
            self._refresh_step_state()
            return

        self.app.apply_config_values(
            nickname=self.nickname_var.get().strip() or "Player",
            host=self.host_var.get().strip() or DEFAULT_HOST,
            port_text=self.port_var.get().strip() or str(DEFAULT_PORT),
            gta_path=self.gta_path_var.get().strip(),
        )
        self.app.save_current_config()
        self.app.show_setup_complete_banner()
        self._close()


@dataclass
class ValidationResult:
    ok: bool
    error: str = ""


@dataclass
class GameValidationResult:
    ok: bool
    missing: list[str]


class LauncherApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("CoopAndreas Launcher")
        self.root.geometry("760x560")

        self.server_process: subprocess.Popen[str] | None = None
        self.server_reader_threads: list[threading.Thread] = []

        self.config = self._load_config()

        self.nickname_var = tk.StringVar(value=self.config.nickname)
        self.host_var = tk.StringVar(value=self.config.host)
        self.port_var = tk.StringVar(value=str(self.config.port))
        self.gta_path_var = tk.StringVar(value=self.config.gta_path)

        self.server_running_var = tk.StringVar(value="No")
        self.port_bound_var = tk.StringVar(value="No")
        self.last_error_var = tk.StringVar(value="None")

        self.setup_banner_var = tk.StringVar(value="")

        self._build_ui()
        self._bind_persistence_events()

        self.root.protocol("WM_DELETE_WINDOW", self._on_close)
        self._update_status()
        self._maybe_start_wizard()

    def _build_ui(self) -> None:
        self.wrapper = ttk.Frame(self.root, padding=12)
        self.wrapper.pack(fill=tk.BOTH, expand=True)

        self.setup_banner = ttk.Label(
            self.wrapper,
            textvariable=self.setup_banner_var,
            anchor="w",
            padding=6,
            foreground="#1a7f37",
        )

        self.setup_banner.pack(fill=tk.X, pady=(0, 8))

        config_frame = ttk.LabelFrame(self.wrapper, text="Connection + Game Config", padding=10)
        config_frame.pack(fill=tk.X)

        ttk.Label(config_frame, text="Nickname").grid(row=0, column=0, sticky="w")
        ttk.Entry(config_frame, textvariable=self.nickname_var, width=24).grid(row=0, column=1, sticky="we", padx=(8, 16))

        ttk.Label(config_frame, text="Host / IP").grid(row=0, column=2, sticky="w")
        ttk.Entry(config_frame, textvariable=self.host_var, width=20).grid(row=0, column=3, sticky="we", padx=(8, 0))

        ttk.Label(config_frame, text="Port").grid(row=1, column=0, sticky="w", pady=(8, 0))
        ttk.Entry(config_frame, textvariable=self.port_var, width=10).grid(row=1, column=1, sticky="w", padx=(8, 16), pady=(8, 0))

        ttk.Label(config_frame, text="GTA:SA path").grid(row=1, column=2, sticky="w", pady=(8, 0))
        ttk.Entry(config_frame, textvariable=self.gta_path_var).grid(row=1, column=3, sticky="we", padx=(8, 8), pady=(8, 0))
        ttk.Button(config_frame, text="Browse", command=self._browse_gta_path).grid(row=1, column=4, sticky="e", pady=(8, 0))

        config_frame.columnconfigure(1, weight=1)
        config_frame.columnconfigure(3, weight=2)

        controls = ttk.Frame(self.wrapper, padding=(0, 10, 0, 4))
        controls.pack(fill=tk.X)
        ttk.Button(controls, text="Start Local Server", command=self.start_server).pack(side=tk.LEFT)
        ttk.Button(controls, text="Stop Server", command=self.stop_server).pack(side=tk.LEFT, padx=8)
        ttk.Button(controls, text="Launch Game", command=self.launch_game).pack(side=tk.RIGHT)

        status = ttk.LabelFrame(self.wrapper, text="Status", padding=10)
        status.pack(fill=tk.X)

        ttk.Label(status, text="Server running:").grid(row=0, column=0, sticky="w")
        ttk.Label(status, textvariable=self.server_running_var).grid(row=0, column=1, sticky="w", padx=(8, 24))

        ttk.Label(status, text="Port bound:").grid(row=0, column=2, sticky="w")
        ttk.Label(status, textvariable=self.port_bound_var).grid(row=0, column=3, sticky="w", padx=(8, 24))

        ttk.Label(status, text="Last error:").grid(row=1, column=0, sticky="w", pady=(8, 0))
        ttk.Label(status, textvariable=self.last_error_var).grid(row=1, column=1, columnspan=3, sticky="w", padx=(8, 0), pady=(8, 0))

        logs = ttk.LabelFrame(self.wrapper, text="Server logs", padding=10)
        logs.pack(fill=tk.BOTH, expand=True, pady=(10, 0))

        self.log_text = tk.Text(logs, height=16, wrap="word")
        self.log_text.pack(fill=tk.BOTH, expand=True)
        self.log_text.configure(state=tk.DISABLED)

    def _bind_persistence_events(self) -> None:
        for var in (self.nickname_var, self.host_var, self.port_var, self.gta_path_var):
            var.trace_add("write", self._autosave)

    def _autosave(self, *_args: object) -> None:
        self.save_current_config()
        self._update_status()

    def _load_config(self) -> LauncherConfig:
        if not CONFIG_PATH.exists():
            return LauncherConfig()
        try:
            data = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
            return LauncherConfig(
                nickname=str(data.get("nickname", "Player")),
                host=str(data.get("host", DEFAULT_HOST)),
                port=int(data.get("port", DEFAULT_PORT)),
                gta_path=str(data.get("gta_path", "")),
            )
        except Exception:
            return LauncherConfig()

    def apply_config_values(self, nickname: str, host: str, port_text: str, gta_path: str) -> None:
        self.nickname_var.set(nickname)
        self.host_var.set(host)
        self.port_var.set(port_text)
        self.gta_path_var.set(gta_path)

    def save_current_config(self) -> None:
        try:
            config = LauncherConfig(
                nickname=self.nickname_var.get().strip() or "Player",
                host=self.host_var.get().strip() or DEFAULT_HOST,
                port=self._read_port(or_default=True),
                gta_path=self.gta_path_var.get().strip(),
            )
            CONFIG_PATH.write_text(json.dumps(asdict(config), indent=2), encoding="utf-8")
        except Exception as exc:
            self._set_error(f"Save config failed: {exc}")

    def _read_port(self, or_default: bool = False) -> int:
        raw = self.port_var.get().strip()
        if not raw and or_default:
            return DEFAULT_PORT
        port = int(raw)
        if port < 1 or port > 65535:
            raise ValueError("Port must be between 1 and 65535")
        return port

    def _browse_gta_path(self) -> None:
        selected = filedialog.askdirectory(title="Select GTA:SA directory")
        if selected:
            self.gta_path_var.set(selected)

    def _append_log(self, line: str) -> None:
        self.log_text.configure(state=tk.NORMAL)
        self.log_text.insert(tk.END, line.rstrip("\n") + "\n")
        self.log_text.see(tk.END)
        self.log_text.configure(state=tk.DISABLED)

    def _set_error(self, message: str) -> None:
        self.last_error_var.set(message)
        self._append_log(f"[error] {message}")

    def _resolve_server_binary(self) -> Path | None:
        repo_root = Path(__file__).resolve().parent.parent
        candidates = [
            repo_root / "build" / "windows" / "x86" / "release" / "server.exe",
            repo_root / "build" / "server.exe",
            repo_root / "server.exe",
        ]
        for candidate in candidates:
            if candidate.exists():
                return candidate
        return None

    def start_server(self) -> None:
        if self.server_process and self.server_process.poll() is None:
            self._set_error("Server is already running")
            return

        try:
            port = self._read_port()
        except Exception as exc:
            self._set_error(str(exc))
            messagebox.showerror("Invalid port", str(exc))
            return

        server_binary = self._resolve_server_binary()
        if not server_binary:
            msg = "Could not find server.exe. Build the server target first."
            self._set_error(msg)
            messagebox.showerror("Missing server.exe", msg)
            return

        try:
            self._ensure_server_config(server_binary.parent, port)
            self.server_process = subprocess.Popen(
                [str(server_binary)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1,
            )
            self._append_log(f"[info] server started: {server_binary}")
            self.last_error_var.set("None")
            self._start_log_stream(self.server_process.stdout, "stdout")
            self._start_log_stream(self.server_process.stderr, "stderr")
            self._update_status()
        except Exception as exc:
            self._set_error(f"Failed to start server: {exc}")

    def _ensure_server_config(self, server_dir: Path, port: int) -> None:
        config_path = server_dir / "server-config.ini"
        lines: list[str] = []
        if config_path.exists():
            lines = config_path.read_text(encoding="utf-8").splitlines()

        updated = False
        for i, line in enumerate(lines):
            if line.strip().lower().startswith("port"):
                lines[i] = f"port = {port}"
                updated = True
                break

        if not updated:
            lines.insert(0, f"port = {port}")
            if len(lines) == 1:
                lines.append("maxplayers = 32")

        config_path.write_text("\n".join(lines).strip() + "\n", encoding="utf-8")

    def _start_log_stream(self, stream, label: str) -> None:
        if stream is None:
            return

        def _reader() -> None:
            for line in stream:
                self.root.after(0, self._append_log, f"[{label}] {line.rstrip()}")

        thread = threading.Thread(target=_reader, daemon=True)
        thread.start()
        self.server_reader_threads.append(thread)

    def stop_server(self) -> None:
        if not self.server_process or self.server_process.poll() is not None:
            self._append_log("[info] server is not running")
            self._update_status()
            return

        self.server_process.terminate()
        try:
            self.server_process.wait(timeout=5)
            self._append_log("[info] server stopped")
        except subprocess.TimeoutExpired:
            self.server_process.kill()
            self._append_log("[info] server force-killed")
        finally:
            self._update_status()

    def _check_port_bound(self) -> bool:
        host = self.host_var.get().strip() or DEFAULT_HOST
        try:
            port = self._read_port()
        except Exception:
            return False
        try:
            with socket.create_connection((host, port), timeout=0.25):
                return True
        except OSError:
            return False

    def _required_game_files(self, gta_path: Path) -> list[tuple[str, Path]]:
        return [
            ("gta_sa.exe", gta_path / "gta_sa.exe"),
            ("CoopAndreasSA.dll", gta_path / "CoopAndreasSA.dll"),
            ("eax.dll", gta_path / "eax.dll"),
        ]

    def validate_connection_settings(self, nickname: str, host: str, port_text: str) -> ValidationResult:
        clean_nickname = nickname.strip()
        clean_host = host.strip()
        if not clean_nickname:
            return ValidationResult(False, "Nickname is required.")
        if not clean_host:
            return ValidationResult(False, "Host is required.")

        try:
            port = int(port_text.strip())
        except ValueError:
            return ValidationResult(False, "Port must be a number.")

        if port < 1 or port > 65535:
            return ValidationResult(False, "Port must be between 1 and 65535.")
        return ValidationResult(True)

    def validate_game_install(self, gta_path_text: str) -> GameValidationResult:
        gta_dir = Path(gta_path_text.strip())
        if not gta_path_text.strip():
            return GameValidationResult(False, ["GTA path is required."])
        if not gta_dir.exists():
            return GameValidationResult(False, ["GTA path does not exist."])

        missing: list[str] = []
        for label, path in self._required_game_files(gta_dir):
            if not path.exists():
                missing.append(label)

        coop_script_dir = gta_dir / "CoopAndreas"
        has_compiled_script = (coop_script_dir / "main.scm").exists() or (coop_script_dir / "script.img").exists()
        if not has_compiled_script:
            missing.append("CoopAndreas/main.scm or CoopAndreas/script.img")

        return GameValidationResult(ok=not missing, missing=missing)

    def launch_game(self) -> None:
        game_validation = self.validate_game_install(self.gta_path_var.get())
        if not game_validation.ok:
            msg = "Missing required files:\n- " + "\n- ".join(game_validation.missing)
            self._set_error(msg.replace("\n", " "))
            messagebox.showerror("Missing required files", msg)
            return

        try:
            gta_dir = Path(self.gta_path_var.get().strip())
            subprocess.Popen([str(gta_dir / "gta_sa.exe")], cwd=str(gta_dir))
            self._append_log("[info] launched gta_sa.exe")
            self.last_error_var.set("None")
        except Exception as exc:
            self._set_error(f"Failed to launch game: {exc}")

    def config_requires_wizard(self) -> bool:
        if not CONFIG_PATH.exists():
            return True

        conn_result = self.validate_connection_settings(
            self.nickname_var.get(),
            self.host_var.get(),
            self.port_var.get(),
        )
        game_result = self.validate_game_install(self.gta_path_var.get())
        return not (conn_result.ok and game_result.ok)

    def _maybe_start_wizard(self) -> None:
        if self.config_requires_wizard():
            self.root.after(100, lambda: SetupWizard(self))

    def show_setup_complete_banner(self) -> None:
        self.setup_banner_var.set("Setup complete. You can now start the server and launch the game.")

    def _update_status(self) -> None:
        running = self.server_process is not None and self.server_process.poll() is None
        self.server_running_var.set("Yes" if running else "No")
        self.port_bound_var.set("Yes" if self._check_port_bound() else "No")
        self.root.after(1000, self._update_status)

    def _on_close(self) -> None:
        self.save_current_config()
        if self.server_process and self.server_process.poll() is None:
            self.server_process.terminate()
        self.root.destroy()


def main() -> None:
    root = tk.Tk()
    LauncherApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
