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

        self._build_ui()
        self._bind_persistence_events()

        self.root.protocol("WM_DELETE_WINDOW", self._on_close)
        self._update_status()

    def _build_ui(self) -> None:
        wrapper = ttk.Frame(self.root, padding=12)
        wrapper.pack(fill=tk.BOTH, expand=True)

        config_frame = ttk.LabelFrame(wrapper, text="Connection + Game Config", padding=10)
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

        controls = ttk.Frame(wrapper, padding=(0, 10, 0, 4))
        controls.pack(fill=tk.X)
        ttk.Button(controls, text="Start Local Server", command=self.start_server).pack(side=tk.LEFT)
        ttk.Button(controls, text="Stop Server", command=self.stop_server).pack(side=tk.LEFT, padx=8)
        ttk.Button(controls, text="Launch Game", command=self.launch_game).pack(side=tk.RIGHT)

        status = ttk.LabelFrame(wrapper, text="Status", padding=10)
        status.pack(fill=tk.X)

        ttk.Label(status, text="Server running:").grid(row=0, column=0, sticky="w")
        ttk.Label(status, textvariable=self.server_running_var).grid(row=0, column=1, sticky="w", padx=(8, 24))

        ttk.Label(status, text="Port bound:").grid(row=0, column=2, sticky="w")
        ttk.Label(status, textvariable=self.port_bound_var).grid(row=0, column=3, sticky="w", padx=(8, 24))

        ttk.Label(status, text="Last error:").grid(row=1, column=0, sticky="w", pady=(8, 0))
        ttk.Label(status, textvariable=self.last_error_var).grid(row=1, column=1, columnspan=3, sticky="w", padx=(8, 0), pady=(8, 0))

        logs = ttk.LabelFrame(wrapper, text="Server logs", padding=10)
        logs.pack(fill=tk.BOTH, expand=True, pady=(10, 0))

        self.log_text = tk.Text(logs, height=16, wrap="word")
        self.log_text.pack(fill=tk.BOTH, expand=True)
        self.log_text.configure(state=tk.DISABLED)

    def _bind_persistence_events(self) -> None:
        for var in (self.nickname_var, self.host_var, self.port_var, self.gta_path_var):
            var.trace_add("write", self._autosave)

    def _autosave(self, *_args: object) -> None:
        self._save_config()
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

    def _save_config(self) -> None:
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

    def launch_game(self) -> None:
        gta_dir = Path(self.gta_path_var.get().strip())
        if not gta_dir.exists():
            msg = "GTA path does not exist. Set a valid GTA:SA directory first."
            self._set_error(msg)
            messagebox.showerror("Invalid GTA path", msg)
            return

        missing: list[str] = []
        for label, path in self._required_game_files(gta_dir):
            if not path.exists():
                missing.append(label)

        coop_script_dir = gta_dir / "CoopAndreas"
        has_compiled_script = (coop_script_dir / "main.scm").exists() or (coop_script_dir / "script.img").exists()
        if not has_compiled_script:
            missing.append("CoopAndreas/main.scm or CoopAndreas/script.img")

        if missing:
            msg = "Missing required files:\n- " + "\n- ".join(missing)
            self._set_error(msg.replace("\n", " "))
            messagebox.showerror("Missing required files", msg)
            return

        try:
            subprocess.Popen([str(gta_dir / "gta_sa.exe")], cwd=str(gta_dir))
            self._append_log("[info] launched gta_sa.exe")
            self.last_error_var.set("None")
        except Exception as exc:
            self._set_error(f"Failed to launch game: {exc}")

    def _update_status(self) -> None:
        running = self.server_process is not None and self.server_process.poll() is None
        self.server_running_var.set("Yes" if running else "No")
        self.port_bound_var.set("Yes" if self._check_port_bound() else "No")
        self.root.after(1000, self._update_status)

    def _on_close(self) -> None:
        self._save_config()
        if self.server_process and self.server_process.poll() is None:
            self.server_process.terminate()
        self.root.destroy()


def main() -> None:
    root = tk.Tk()
    LauncherApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
