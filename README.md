  <h1>o3ShellTaskMonitor</h1>
  <p>
  Current version: 1.7.0 | Rev 0.1.0<br>
  License: MIT License<br>
  Developer: openw3rk INVENT<br>
  Copyright: (c) openw3rk INVENT<br>
  </p>

  <h2>Description</h2>
  <p>Cross-platform CPU and memory usage monitor with process list, supporting Windows consoles.</p>

  <h2>Features</h2>
  <ul>
    <li>Displays real-time <strong>CPU usage</strong></li>
    <li>Displays <strong>RAM usage</strong></li>
    <li>Lists all running processes with:
      <ul>
        <li>PID</li>
        <li>Process name</li>
        <li>RAM usage (MB)</li>
      </ul>
    </li>
    <li>Supports <strong>colored output</strong></li>
    <li>Works on:
      <ul>
        <li>Windows (cmd or PowerShell) or Windows Server whitout Desktop-environment</li>
      </ul>
  
  <h2>Compilation</h2>

  <h3>Basic (No Icon)</h3>
  <h4>Windows</h4>
  <pre><code>gcc main.c -o o3ShellTaskMonitor.exe -lpsapi</code></pre>

  <p><em><code>-lpsapi</code> is required for <code>GetProcessMemoryInfo</code> on Windows.</em></p>

  <h3>With Icon</h3>

  <p>1. Create a resource file named <code>resource.rc</code>:</p>
  <pre><code>IDI_ICON1 ICON "icon.ico"</code></pre>

  <p>2. Compile resource file:</p>
  <pre><code>windres resource.rc -o resource.o</code></pre>

  <p>3. Compile program with the icon:</p>
  <pre><code>gcc main.c resource.o -o o3ShellTaskMonitor.exe -lpsapi</code></pre>

  <p>4. Program is ready</p>

  <p>5. Run it</p>

  <h2>Example Output</h2>
  <pre><code>
o3ShellTaskMonitor - Version 1.7.0 | Rev 0.1.0
-----------------------------------------------------
Copyright (c) openw3rk INVENT
https://openw3rk.de
develop@openw3rk.de
-----------------------------------------------------
o3ShellTaskMonitor comes with ABSOLUTELY NO WARRANTY.
-----------------------------------------------------
<br>
    
CPU-usage: 12.43%   RAM-usage: 47.12%
PID     Processname               RAM (MB)
1040    program1.exe                 55
5432    program2.exe                 624
...
</code></pre>

  <h2>License</h2>
  <p>MIT License – Note the LICENSE file.</p>
  <h4>Copyright (c) openw3rk INVENT</h4>

</body>
</html>
