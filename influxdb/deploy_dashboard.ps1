# =====================================================================
#  Deploy dashboard Smart Agriculture ke InfluxDB v2 via API.
#  Membaca dashboard_smart_agriculture.json (format template),
#  mengubahnya menjadi payload API, lalu membuat dashboard.
#
#  Pakai:
#    ./deploy_dashboard.ps1
#    ./deploy_dashboard.ps1 -Token "xxx" -Url "http://localhost:8086" -Org "smart-agri"
# =====================================================================
param(
    [string]$Url   = "http://localhost:8086",
    [string]$Token = "smart-agri-dev-token",
    [string]$Org   = "smart-agri",
    [string]$TemplateFile = "$PSScriptRoot\dashboard_smart_agriculture.json"
)

$headers = @{ Authorization = "Token $Token"; "Content-Type" = "application/json" }

# 1. Ambil orgID
Write-Host ">> Ambil orgID untuk '$Org'..." -ForegroundColor Cyan
$orgs = (Invoke-WebRequest -Uri "$Url/api/v2/orgs?org=$Org" -Headers $headers -UseBasicParsing).Content | ConvertFrom-Json
$orgID = $orgs.orgs[0].id
Write-Host "   orgID = $orgID"

# 2. Baca template (query sudah memakai bucket "agriculture" langsung,
#    tidak perlu variabel dashboard)
$tmpl = Get-Content $TemplateFile -Raw | ConvertFrom-Json
$cells = $tmpl.content.cells

# 3. Ubah cell template -> cell API.
#    Template UI pakai view {name, type, properties}.
#    API InfluxDB butuh    {name, properties:{shape:"chronograf-v2", type:<...>, ...}}.
#    Juga 'queries' level-cell dibuang (query sudah ada di view.properties.queries).
$typeMap = @{ "singleStat" = "single-stat"; "xy" = "xy"; "gauge" = "gauge"; "table" = "table" }

$apiCells = @()
foreach ($c in $cells) {
    $t = $c.view.type
    if ($typeMap.ContainsKey($t)) { $t = $typeMap[$t] }

    $newProps = [ordered]@{ shape = "chronograf-v2"; type = $t }
    foreach ($p in $c.view.properties.PSObject.Properties) {
        $newProps[$p.Name] = $p.Value
    }

    $apiCells += [ordered]@{
        x    = $c.x
        y    = $c.y
        w    = $c.w
        h    = $c.h
        view = [ordered]@{
            name       = $c.view.name
            properties = $newProps
        }
    }
}

# 4. Buat dashboard (view TIDAK ikut tersimpan via POST - perlu PATCH).
Write-Host ">> Membuat dashboard..." -ForegroundColor Cyan
$payload = [ordered]@{
    name        = $tmpl.content.label
    description = $tmpl.content.description
    orgID       = $orgID
    cells       = @()          # cell dibuat satu per satu di bawah
} | ConvertTo-Json -Depth 100

$resp = Invoke-WebRequest -Uri "$Url/api/v2/dashboards" -Method Post -Headers $headers -Body $payload -UseBasicParsing
$dash = $resp.Content | ConvertFrom-Json
Write-Host "   dashboardId = $($dash.id)"

# 5. Buat tiap cell lalu PATCH view-nya (langkah wajib agar panel tampil).
Write-Host ">> Menambahkan panel..." -ForegroundColor Cyan
for ($i = 0; $i -lt $apiCells.Count; $i++) {
    $cell = $apiCells[$i]
    $cellBody = [ordered]@{ x = $cell.x; y = $cell.y; w = $cell.w; h = $cell.h } | ConvertTo-Json
    $cresp = Invoke-WebRequest -Uri "$Url/api/v2/dashboards/$($dash.id)/cells" -Method Post -Headers $headers -Body $cellBody -UseBasicParsing
    $cellId = ($cresp.Content | ConvertFrom-Json).id

    $viewBody = [ordered]@{ name = $cell.view.name; properties = $cell.view.properties } | ConvertTo-Json -Depth 100
    Invoke-WebRequest -Uri "$Url/api/v2/dashboards/$($dash.id)/cells/$cellId/view" -Method Patch -Headers $headers -Body $viewBody -UseBasicParsing | Out-Null
    Write-Host ("   [{0}] {1}" -f $cell.view.properties.type, $cell.view.name)
}

Write-Host ""
Write-Host "====================================================" -ForegroundColor Green
Write-Host "  Dashboard dibuat!" -ForegroundColor Green
Write-Host "  Nama : $($dash.name)"
Write-Host "  ID   : $($dash.id)"
Write-Host "  Buka : $Url/orgs/$orgID/dashboards/$($dash.id)"
Write-Host "====================================================" -ForegroundColor Green
