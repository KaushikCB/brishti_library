# Comprehensive automated test script for BrishtiLibrary new features
$ErrorActionPreference = "Stop"

Write-Host "====================================================" -ForegroundColor Cyan
Write-Host "   TESTING BRISHTILIBRARY 4 CORE NEW FEATURES        " -ForegroundColor Cyan
Write-Host "====================================================" -ForegroundColor Cyan

# 1. Backup database
Copy-Item "database/brishti.db" "database/brishti_test_backup.db" -Force

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

    # 1.4 Perform first-time admin setup
    $setupPayload = @{ username = "testadmin"; password = "securePassword123" } | ConvertTo-Json
    $setupResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/setup" -Method Post -Body $setupPayload -ContentType "application/json" -UseBasicParsing -SessionVariable adminSession
    $setupData = $setupResp.Content | ConvertFrom-Json
    Assert-Test "First-time setup succeeds" ($setupResp.StatusCode -eq 200 -and $setupData.status -eq "success")

    # 1.5 Second setup attempt is blocked
    try {
        $setupResp2 = Invoke-WebRequest -Uri "$baseUrl/api/admin/setup" -Method Post -Body $setupPayload -ContentType "application/json" -UseBasicParsing
        Assert-Test "Repeated setup attempt is blocked" ($false)
    } catch {
        Assert-Test "Repeated setup attempt is blocked (400 Bad Request)" ($_.Exception.Response.StatusCode.value__ -eq 400)
    }

    # 1.6 Setup status now reports completed
    $statusRes2 = Invoke-RestMethod -Uri "$baseUrl/api/admin/setup-status" -UseBasicParsing
    Assert-Test "setup_completed is now true" ($statusRes2.setup_completed -eq $true)

    # 1.7 Login with wrong password fails
    try {
        $wrongLogin = @{ username = "testadmin"; password = "wrongPassword!" } | ConvertTo-Json
        Invoke-WebRequest -Uri "$baseUrl/api/admin/login" -Method Post -Body $wrongLogin -ContentType "application/json" -UseBasicParsing
        Assert-Test "Wrong password login rejected with 401" ($false)
    } catch {
        Assert-Test "Wrong password login rejected with 401" ($_.Exception.Response.StatusCode.value__ -eq 401)
    }

    # 1.8 Login with correct password succeeds
    $correctLogin = @{ username = "testadmin"; password = "securePassword123" } | ConvertTo-Json
    $loginResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/login" -Method Post -Body $correctLogin -ContentType "application/json" -UseBasicParsing -WebSession $adminSession
    $loginData = $loginResp.Content | ConvertFrom-Json
    Assert-Test "Admin login with correct password succeeds" ($loginResp.StatusCode -eq 200 -and $loginData.status -eq "success")
    Assert-Test "Admin login returns user role 'admin'" ($loginData.user.role -eq "admin" -or $loginData.role -eq "admin")

    # 1.9 Change admin password
    $pwPayload = @{ current_password = "securePassword123"; new_password = "newAdminSecret999" } | ConvertTo-Json
    $pwResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/change-password" -Method Post -Body $pwPayload -ContentType "application/json" -UseBasicParsing -WebSession $adminSession
    $pwData = $pwResp.Content | ConvertFrom-Json
    Assert-Test "Admin password change succeeds" ($pwResp.StatusCode -eq 200 -and $pwData.status -eq "success")

    # 1.10 New password works for login
    $newLogin = @{ username = "testadmin"; password = "newAdminSecret999" } | ConvertTo-Json
    $newLoginResp = Invoke-WebRequest -Uri "$baseUrl/api/admin/login" -Method Post -Body $newLogin -ContentType "application/json" -UseBasicParsing -WebSession $adminSession
    Assert-Test "New password successfully logs in" ($newLoginResp.StatusCode -eq 200)

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
    $prodWithImg = @{
        type = "book"
        sku = "TEST-IMG-001"
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
    $savedProd = $allProds | Where-Object { $_.sku -eq "TEST-IMG-001" } | Select-Object -First 1
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

    # --- REGRESSION CHECK: STOREFRONT CART & CSE214 ALGORITHMS ---
    Write-Host "`n--- Testing Regression: Storefront & Algorithms ---" -ForegroundColor Yellow
    
    $algoSortRes = Invoke-RestMethod -Uri "$baseUrl/api/algorithms/sort?algorithm=quick&size=10"
    Assert-Test "C Algorithm Quick Sort API works" ($algoSortRes.status -eq "success" -and $algoSortRes.comparisons -gt 0)

    $algoDijkstraRes = Invoke-RestMethod -Uri "$baseUrl/api/algorithms/dijkstra?source=0"
    Assert-Test "C Algorithm Dijkstra API works" ($algoDijkstraRes.status -eq "success" -and $algoDijkstraRes.nodes.Count -gt 0)

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
}

Write-Host "`n====================================================" -ForegroundColor Cyan
Write-Host "   RESULTS: Total: $($passed + $failed) | Passed: $passed | Failed: $failed" -ForegroundColor Cyan
Write-Host "====================================================" -ForegroundColor Cyan

if ($failed -gt 0) {
    exit 1
} else {
    exit 0
}
