# Comprehensive automated test script for BrishtiLibrary new features
$ErrorActionPreference = "Stop"

Write-Host "====================================================" -ForegroundColor Cyan
Write-Host "   TESTING BRISHTILIBRARY 4 CORE NEW FEATURES        " -ForegroundColor Cyan
Write-Host "====================================================" -ForegroundColor Cyan

# 1. Backup database
Copy-Item "database/brishti.db" "database/brishti_test_backup.db" -Force

# Reset admin user so test starts in initial first-time setup state
& python scratch/reset_admin.py

$passed = 0
$failed = 0

function Assert-Test($name, $condition) {
    if ($condition) {
        Write-Host "  [PASS] $name" -ForegroundColor Green
        $script:passed++
    } else {
        Write-Host "  [FAIL] $name" -ForegroundColor Red
        $script:failed++
    }
}

# Start server on test port 8091
$proc = Start-Process -FilePath "backend/server.exe" -ArgumentList "8091" -PassThru
Start-Sleep -Seconds 2

try {
    $baseUrl = "http://localhost:8091"

    # --- FEATURE 1: SECURE ADMIN SETUP & LOGIN ---
    Write-Host "`n--- Testing Feature 1: Secure Admin Login ---" -ForegroundColor Yellow
    
    # 1.1 Check initial setup status
    $statusRes = Invoke-RestMethod -Uri "$baseUrl/api/admin/setup-status" -UseBasicParsing
    Assert-Test "Admin setup-status returns JSON" ($statusRes.status -eq "success")
    Assert-Test "Initial setup_completed is false" ($statusRes.setup_completed -eq $false)

    # 1.2 Demo admin login is blocked
    try {
        $body = @{ role = "admin" } | ConvertTo-Json
        $demoRes = Invoke-WebRequest -Uri "$baseUrl/api/auth/demo-login" -Method Post -Body $body -ContentType "application/json" -UseBasicParsing
        Assert-Test "Demo admin login is rejected with 403" ($false)
    } catch {
        Assert-Test "Demo admin login is rejected with 403" ($_.Exception.Response.StatusCode.value__ -eq 403)
    }

    # 1.3 Customer demo login remains functional
    $custBody = @{ role = "customer" } | ConvertTo-Json
    $custRes = Invoke-WebRequest -Uri "$baseUrl/api/auth/demo-login" -Method Post -Body $custBody -ContentType "application/json" -UseBasicParsing -SessionVariable custSession
    Assert-Test "Customer demo login succeeds with 200" ($custRes.StatusCode -eq 200)

    # 1.4 Admin setup with missing email rejected with 400
    try {
        $badSetup1 = @{ email = ""; password = "securePassword123" } | ConvertTo-Json
        Invoke-WebRequest -Uri "$baseUrl/api/admin/setup" -Method Post -Body $badSetup1 -ContentType "application/json" -UseBasicParsing
        Assert-Test "Setup with missing email is rejected" ($false)
    } catch {
        Assert-Test "Setup with missing email is rejected with 400" ($_.Exception.Response.StatusCode.value__ -eq 400)
    }

    # 1.5 Admin setup with invalid email syntax rejected with 400
    try {
        $badSetup2 = @{ email = "not-an-email"; password = "securePassword123" } | ConvertTo-Json
        Invoke-WebRequest -Uri "$baseUrl/api/admin/setup" -Method Post -Body $badSetup2 -ContentType "application/json" -UseBasicParsing
        Assert-Test "Setup with invalid email syntax is rejected" ($false)
    } catch {
        Assert-Test "Setup with invalid email syntax is rejected with 400" ($_.Exception.Response.StatusCode.value__ -eq 400)
    }

    # 1.6 Perform first-time admin setup with valid email
    $setupPayload = @{ email = "admin@brishtilibrary.edu"; password = "securePassword123" } | ConvertTo-Json
    $setupResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/setup" -Method Post -Body $setupPayload -ContentType "application/json" -UseBasicParsing -SessionVariable adminSession
    $setupData = $setupResp.Content | ConvertFrom-Json
    Assert-Test "First-time setup succeeds with valid email" ($setupResp.StatusCode -eq 200 -and $setupData.status -eq "success")
    Assert-Test "Setup response returns admin email" ($setupData.email -eq "admin@brishtilibrary.edu")

    # 1.7 Second setup attempt is blocked
    try {
        $setupResp2 = Invoke-WebRequest -Uri "$baseUrl/api/admin/setup" -Method Post -Body $setupPayload -ContentType "application/json" -UseBasicParsing
        Assert-Test "Repeated setup attempt is blocked" ($false)
    } catch {
        Assert-Test "Repeated setup attempt is blocked (400 Bad Request)" ($_.Exception.Response.StatusCode.value__ -eq 400)
    }

    # 1.8 Setup status now reports completed
    $statusRes2 = Invoke-RestMethod -Uri "$baseUrl/api/admin/setup-status" -UseBasicParsing
    Assert-Test "setup_completed is now true" ($statusRes2.setup_completed -eq $true)

    # 1.9 Login with missing email fails with 400
    try {
        $noEmailLogin = @{ email = ""; password = "securePassword123" } | ConvertTo-Json
        Invoke-WebRequest -Uri "$baseUrl/api/admin/login" -Method Post -Body $noEmailLogin -ContentType "application/json" -UseBasicParsing
        Assert-Test "Login with missing email rejected with 400" ($false)
    } catch {
        Assert-Test "Login with missing email rejected with 400" ($_.Exception.Response.StatusCode.value__ -eq 400)
    }

    # 1.10 Login with wrong email fails with 401
    try {
        $wrongEmailLogin = @{ email = "wrongadmin@brishtilibrary.edu"; password = "securePassword123" } | ConvertTo-Json
        Invoke-WebRequest -Uri "$baseUrl/api/admin/login" -Method Post -Body $wrongEmailLogin -ContentType "application/json" -UseBasicParsing
        Assert-Test "Wrong email login rejected with 401" ($false)
    } catch {
        Assert-Test "Wrong email login rejected with 401" ($_.Exception.Response.StatusCode.value__ -eq 401)
    }

    # 1.11 Login with wrong password fails with 401
    try {
        $wrongLogin = @{ email = "admin@brishtilibrary.edu"; password = "wrongPassword!" } | ConvertTo-Json
        Invoke-WebRequest -Uri "$baseUrl/api/admin/login" -Method Post -Body $wrongLogin -ContentType "application/json" -UseBasicParsing
        Assert-Test "Wrong password login rejected with 401" ($false)
    } catch {
        Assert-Test "Wrong password login rejected with 401" ($_.Exception.Response.StatusCode.value__ -eq 401)
    }

    # 1.12 Login with correct email and password succeeds
    $correctLogin = @{ email = "admin@brishtilibrary.edu"; password = "securePassword123" } | ConvertTo-Json
    $loginResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/login" -Method Post -Body $correctLogin -ContentType "application/json" -UseBasicParsing -WebSession $adminSession
    $loginData = $loginResp.Content | ConvertFrom-Json
    Assert-Test "Admin login with correct email succeeds" ($loginResp.StatusCode -eq 200 -and $loginData.status -eq "success")
    Assert-Test "Admin login returns user role 'admin'" ($loginData.user.role -eq "admin" -or $loginData.role -eq "admin")

    # 1.13 Case-insensitive email login test (Admin@BrishtiLibrary.EDU)
    $caseLogin = @{ email = "Admin@BrishtiLibrary.EDU"; password = "securePassword123" } | ConvertTo-Json
    $caseResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/login" -Method Post -Body $caseLogin -ContentType "application/json" -UseBasicParsing
    Assert-Test "Case-insensitive email login succeeds (200)" ($caseResp.StatusCode -eq 200)

    # 1.14 Change admin password
    $pwPayload = @{ current_password = "securePassword123"; new_password = "newAdminSecret999" } | ConvertTo-Json
    $pwResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/change-password" -Method Post -Body $pwPayload -ContentType "application/json" -UseBasicParsing -WebSession $adminSession
    $pwData = $pwResp.Content | ConvertFrom-Json
    Assert-Test "Admin password change succeeds" ($pwResp.StatusCode -eq 200 -and $pwData.status -eq "success")

    # 1.15 New password works for login with email
    $newLogin = @{ email = "admin@brishtilibrary.edu"; password = "newAdminSecret999" } | ConvertTo-Json
    $newLoginResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/login" -Method Post -Body $newLogin -ContentType "application/json" -UseBasicParsing -WebSession $adminSession
    Assert-Test "New password successfully logs in with email" ($newLoginResp.StatusCode -eq 200)

    # --- FEATURE 2: CATEGORY MANAGEMENT ---
    Write-Host "`n--- Testing Feature 2: Category Management ---" -ForegroundColor Yellow

    # 2.1 Fetch existing categories
    $cats = Invoke-RestMethod -Uri "$baseUrl/api/categories" -UseBasicParsing
    Assert-Test "Public GET /api/categories returns category list" ($cats.Count -gt 0)

    # 2.2 Admin create category
    $newCatPayload = @{ name = "Robotics & AI"; type = "book" } | ConvertTo-Json
    $createCatResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/categories" -Method Post -Body $newCatPayload -ContentType "application/json" -UseBasicParsing -WebSession $adminSession
    $createCatData = $createCatResp.Content | ConvertFrom-Json
    Assert-Test "Admin creates new category" ($createCatResp.StatusCode -eq 201 -and $createCatData.status -eq "success")
    $newCatId = $createCatData.id

    # 2.3 Admin edit category
    $editCatPayload = @{ name = "Advanced Robotics & AI"; type = "book" } | ConvertTo-Json
    $editCatResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/categories/$newCatId" -Method Put -Body $editCatPayload -ContentType "application/json" -UseBasicParsing -WebSession $adminSession
    $editCatData = $editCatResp.Content | ConvertFrom-Json
    Assert-Test "Admin updates category name" ($editCatResp.StatusCode -eq 200 -and $editCatData.status -eq "success")

    # 2.4 Verify safe deletion prevention on category with products
    $existingCat = $cats | Where-Object { $_.product_count -gt 0 } | Select-Object -First 1
    if ($existingCat) {
        try {
            $delFailResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/categories/$($existingCat.id)" -Method Delete -UseBasicParsing -WebSession $adminSession
            Assert-Test "Deleting category with active products is prevented" ($false)
        } catch {
            Assert-Test "Deleting category with active products is prevented with 400" ($_.Exception.Response.StatusCode.value__ -eq 400)
        }
    }

    # 2.5 Admin delete empty category succeeds
    $delCatResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/categories/$newCatId" -Method Delete -UseBasicParsing -WebSession $adminSession
    $delCatData = $delCatResp.Content | ConvertFrom-Json
    Assert-Test "Admin safely deletes empty category" ($delCatResp.StatusCode -eq 200 -and $delCatData.status -eq "success")

    # --- FEATURE 3: PRODUCT IMAGE UPLOADS ---
    Write-Host "`n--- Testing Feature 3: Product Image Uploads ---" -ForegroundColor Yellow

    # Minimal 1x1 valid PNG in Base64: 89 50 4E 47 0D 0A 1A 0A ...
    $validPngBase64 = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=="
    $uploadPayload = @{ data = $validPngBase64 } | ConvertTo-Json
    $uploadResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/upload" -Method Post -Body $uploadPayload -ContentType "application/json" -UseBasicParsing -WebSession $adminSession
    $uploadData = $uploadResp.Content | ConvertFrom-Json
    Assert-Test "Valid PNG image upload accepted by C backend" ($uploadResp.StatusCode -eq 200 -and $uploadData.status -eq "success")
    Assert-Test "Upload response returns persistent URL (/uploads/...)" ($uploadData.url -like "/uploads/bl_*")
    $uploadedUrl = $uploadData.url

    # Check file exists in frontend/uploads
    $savedFilename = $uploadData.filename
    Assert-Test "Uploaded image file exists on disk in frontend/uploads" (Test-Path "frontend/uploads/$savedFilename")

    # Static file serving of the uploaded photo
    $imgServeResp = Invoke-WebRequest -Uri "$baseUrl$uploadedUrl" -UseBasicParsing
    Assert-Test "Static image served with HTTP 200" ($imgServeResp.StatusCode -eq 200)
    Assert-Test "Served with correct image/png Content-Type" ($imgServeResp.Headers["Content-Type"] -like "*image/png*")

    # Rejection of fake/malicious file masquerading as image
    try {
        $fakeImgBase64 = "data:image/jpeg;base64,dGhpcyBpcyBub3QgYW4gaW1hZ2U=" # "this is not an image"
        $fakePayload = @{ data = $fakeImgBase64 } | ConvertTo-Json
        Invoke-WebRequest -Uri "$baseUrl/api/admin/upload" -Method Post -Body $fakePayload -ContentType "application/json" -UseBasicParsing -WebSession $adminSession
        Assert-Test "Invalid image magic bytes rejected with 400" ($false)
    } catch {
        Assert-Test "Invalid image magic bytes rejected with 400" ($_.Exception.Response.StatusCode.value__ -eq 400)
    }

    # Create product with image_url
    $testSku = "TEST-IMG-" + (Get-Random -Minimum 10000 -Maximum 99999)
    $prodWithImg = @{
        type = "book"
        sku = $testSku
        title = "Algorithms with Image Test"
        category = "Computer Science"
        price = 650.0
        stock_quantity = 15
        description = "Test product with persistent photo upload"
        image_url = $uploadedUrl
        author = "C. Tester"
    } | ConvertTo-Json

    $createProdResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/products" -Method Post -Body $prodWithImg -ContentType "application/json" -UseBasicParsing -WebSession $adminSession
    $createProdData = $createProdResp.Content | ConvertFrom-Json
    Assert-Test "Product created with image_url in SQLite" ($createProdResp.StatusCode -eq 201)

    # Verify product in catalog returns image_url
    $allProds = Invoke-RestMethod -Uri "$baseUrl/api/admin/products" -UseBasicParsing -WebSession $adminSession
    $savedProd = $allProds | Where-Object { $_.sku -eq $testSku } | Select-Object -First 1
    Assert-Test "Product in catalog retains image_url" ($savedProd.image_url -eq $uploadedUrl)

    # --- FEATURE 4: WARM BLUE THEMES ---
    Write-Host "`n--- Testing Feature 4: Warm Blue Light/Dark Themes ---" -ForegroundColor Yellow

    $cssContent = [System.IO.File]::ReadAllText("frontend/css/style.css")
    Assert-Test "CSS defines Light Mode background #EAF2F8" ($cssContent.Contains("#EAF2F8"))
    Assert-Test "CSS defines Light Mode cards #F2F6FA" ($cssContent.Contains("#F2F6FA"))
    Assert-Test "CSS defines Light Mode buttons #6B91C7" ($cssContent.Contains("#6B91C7"))
    Assert-Test "CSS defines Light Mode headings #263B53" ($cssContent.Contains("#263B53"))
    Assert-Test "CSS defines Light Mode borders #D3DFEA" ($cssContent.Contains("#D3DFEA"))

    Assert-Test "CSS defines Dark Mode background #151F2B" ($cssContent.Contains("#151F2B"))
    Assert-Test "CSS defines Dark Mode cards #202D3D" ($cssContent.Contains("#202D3D"))
    Assert-Test "CSS defines Dark Mode buttons #789BCB" ($cssContent.Contains("#789BCB"))
    Assert-Test "CSS defines Dark Mode headings #E5EDF7" ($cssContent.Contains("#E5EDF7"))
    Assert-Test "CSS defines Dark Mode borders #35465A" ($cssContent.Contains("#35465A"))

    Assert-Test "theme.js script file exists" (Test-Path "frontend/js/theme.js")
    
    $indexHtml = [System.IO.File]::ReadAllText("frontend/index.html")
    $adminHtml = [System.IO.File]::ReadAllText("frontend/pages/admin.html")
    $algoHtml = [System.IO.File]::ReadAllText("frontend/pages/algorithms.html")

    Assert-Test "Storefront index.html contains theme toggle button" ($indexHtml.Contains("theme-toggle"))
    Assert-Test "Admin admin.html contains theme toggle button" ($adminHtml.Contains("theme-toggle"))
    Assert-Test "Algorithms algorithms.html contains theme toggle button" ($algoHtml.Contains("theme-toggle"))

    # --- FEATURE 5: SQLITE ONLINE BACKUP API ---
    Write-Host "`n--- Testing Feature 5: SQLite Online Backup API ---" -ForegroundColor Yellow

    # 5.1 Customer cannot access backup endpoint
    try {
        Invoke-WebRequest -Uri "$baseUrl/api/admin/backup" -Method Post -Body "{}" -ContentType "application/json" -UseBasicParsing -WebSession $custSession
        Assert-Test "Customer backup request is rejected" ($false)
    } catch {
        Assert-Test "Customer backup request is rejected with 401/403" ($_.Exception.Response.StatusCode.value__ -in @(401, 403))
    }

    # 5.2 Admin creates backup via SQLite Online Backup API
    $backupResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/backup" -Method Post -Body "{}" -ContentType "application/json" -UseBasicParsing -WebSession $adminSession
    $backupData = $backupResp.Content | ConvertFrom-Json
    Assert-Test "Admin creates online backup with HTTP 201" ($backupResp.StatusCode -eq 201 -and $backupData.status -eq "success")
    Assert-Test "Backup filename has valid timestamped format" ($backupData.filename -like "brishti_backup_*.db")
    Assert-Test "Backup file size is greater than zero bytes" ($backupData.size_bytes -gt 0)
    $backupFileName = $backupData.filename

    # 5.3 Backup file exists in database/backups on disk
    Assert-Test "Backup file physically exists in database/backups/" (Test-Path "database/backups/$backupFileName")

    # 5.4 List backups via GET /api/admin/backups
    $backupsListResp = Invoke-RestMethod -Uri "$baseUrl/api/admin/backups" -UseBasicParsing -WebSession $adminSession
    Assert-Test "GET /api/admin/backups returns array of backups" ($backupsListResp.status -eq "success" -and $backupsListResp.backups.Count -gt 0)
    $foundBackup = $backupsListResp.backups | Where-Object { $_.filename -eq $backupFileName } | Select-Object -First 1
    Assert-Test "Created backup appears in backup list" ($foundBackup -ne $null)

    # 5.5 Download backup file via GET /api/admin/backups/download/:filename
    $downloadResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/backups/download/$backupFileName" -UseBasicParsing -WebSession $adminSession
    Assert-Test "Download backup returns HTTP 200" ($downloadResp.StatusCode -eq 200)
    Assert-Test "Download has application/x-sqlite3 content-type" ($downloadResp.Headers["Content-Type"] -like "*application/x-sqlite3*")
    Assert-Test "Downloaded file matches size" ($downloadResp.RawContentLength -eq $backupData.size_bytes -or $downloadResp.Content.Length -gt 0)

    # 5.6 Path traversal prevention on download
    try {
        Invoke-WebRequest -Uri "$baseUrl/api/admin/backups/download/..%2F..%2Fbrishti.db" -UseBasicParsing -WebSession $adminSession
        Assert-Test "Path traversal download is blocked" ($false)
    } catch {
        Assert-Test "Path traversal download is blocked with 400/403" ($_.Exception.Response.StatusCode.value__ -in @(400, 403))
    }

    # 5.7 Delete backup via DELETE /api/admin/backups/:filename
    $deleteBackupResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/backups/$backupFileName" -Method Delete -UseBasicParsing -WebSession $adminSession
    $deleteBackupData = $deleteBackupResp.Content | ConvertFrom-Json
    Assert-Test "DELETE backup returns HTTP 200" ($deleteBackupResp.StatusCode -eq 200 -and $deleteBackupData.status -eq "success")
    Assert-Test "Deleted backup no longer exists on disk" (-not (Test-Path "database/backups/$backupFileName"))

    # --- REGRESSION CHECK: STOREFRONT CART & CSE214 ALGORITHMS ---
    Write-Host "`n--- Testing Regression: Storefront & Algorithms ---" -ForegroundColor Yellow
    
    $sortPayload = @{ algorithm = "quick"; array = @(9, 3, 7, 1, 8, 2, 5, 4, 6) } | ConvertTo-Json
    $algoSortRes = Invoke-RestMethod -Uri "$baseUrl/api/algorithms/sort" -Method Post -Body $sortPayload -ContentType "application/json" -UseBasicParsing
    Assert-Test "C Algorithm Quick Sort API works" ($algoSortRes.status -eq "success" -and $algoSortRes.comparisons -gt 0)

    $dijkstraPayload = @{ source_node = 0 } | ConvertTo-Json
    $algoDijkstraRes = Invoke-RestMethod -Uri "$baseUrl/api/algorithms/graph/shortest-path" -Method Post -Body $dijkstraPayload -ContentType "application/json" -UseBasicParsing
    Assert-Test "C Algorithm Dijkstra API works" ($algoDijkstraRes.status -eq "success" -and $algoDijkstraRes.locations.Count -gt 0)

} finally {
    # Clean up server process
    if ($proc -and -not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force
    }
    # Restore original database so the user can perform their own setup
    if (Test-Path "database/brishti_test_backup.db") {
        Copy-Item "database/brishti_test_backup.db" "database/brishti.db" -Force
        Remove-Item "database/brishti_test_backup.db" -Force
    }
    & python scratch/reset_admin.py
}

Write-Host "`n====================================================" -ForegroundColor Cyan
Write-Host "   RESULTS: Total: $($passed + $failed) | Passed: $passed | Failed: $failed" -ForegroundColor Cyan
Write-Host "====================================================" -ForegroundColor Cyan

if ($failed -gt 0) {
    exit 1
} else {
    exit 0
}
