# Tambah firewall rule untuk InfluxDB port 8086 (butuh Administrator)
$log = "$env:TEMP\influx_fw_result.txt"
try {
    $existing = Get-NetFirewallRule -DisplayName "InfluxDB 8086" -ErrorAction SilentlyContinue
    if ($existing) {
        Set-NetFirewallRule -DisplayName "InfluxDB 8086" -Direction Inbound -Protocol TCP -LocalPort 8086 -Action Allow -Profile Private -Enabled True
        "UPDATED: rule sudah ada, di-enable" | Out-File $log -Encoding utf8
    } else {
        New-NetFirewallRule -DisplayName "InfluxDB 8086" -Direction Inbound -Protocol TCP -LocalPort 8086 -Action Allow -Profile Private | Out-Null
        "CREATED: rule baru dibuat" | Out-File $log -Encoding utf8
    }
    "SUKSES" | Out-File $log -Append -Encoding utf8
} catch {
    "GAGAL: $($_.Exception.Message)" | Out-File $log -Encoding utf8
}
