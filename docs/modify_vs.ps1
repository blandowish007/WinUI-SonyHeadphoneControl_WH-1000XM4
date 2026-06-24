$log = "C:\Users\XUXIANKUN\Desktop\claudecode_work\2026-06-22-工具开发-XM4状态栏控制\docs\vs-modify.log"
"START " + (Get-Date) | Out-File -FilePath $log -Encoding utf8

# 先关掉可能开着的 VS Installer GUI,避免冲突
Get-Process vs_installer,vs_installershell,setup -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

$p = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\setup.exe"
$args = @(
  'modify',
  '--installPath', 'C:\Program Files\Microsoft Visual Studio\2022\Community',
  '--add', 'Microsoft.VisualStudio.Workload.ManagedDesktop',
  '--add', 'Microsoft.VisualStudio.Workload.NativeDesktop',
  '--includeRecommended',
  '--quiet',
  '--norestart',
  '--wait',
  '--nocache'
)
"running: $p $($args -join ' ')" | Out-File -FilePath $log -Append -Encoding utf8

$proc = Start-Process -FilePath $p -ArgumentList $args -Wait -PassThru
"EXITCODE=$($proc.ExitCode)" | Out-File -FilePath $log -Append -Encoding utf8
"DONE " + (Get-Date) | Out-File -FilePath $log -Append -Encoding utf8
