(function () {
  // Elements - Containers
  const adminAuthView = document.getElementById("admin-auth-view");
  const cardAdminSetup = document.getElementById("card-admin-setup");
  const cardAdminLogin = document.getElementById("card-admin-login");
  const adminDashboardContainer = document.getElementById("admin-dashboard-container");

  // Elements - Header & User
  const adminUserPill = document.getElementById("admin-user-pill");
  const adminLogoutBtn = document.getElementById("admin-logout-btn");
  const btnOpenChangePw = document.getElementById("btn-open-change-pw");

  // Elements - Setup & Login Forms
  const formAdminSetup = document.getElementById("form-admin-setup");
  const setupAlert = document.getElementById("setup-alert");
  const formAdminLogin = document.getElementById("form-admin-login");
  const loginAlert = document.getElementById("login-alert");

  // Elements - Stats
  const statOrdersCount = document.getElementById("stat-orders-count");
  const statRevenue = document.getElementById("stat-revenue");
  const statProductsCount = document.getElementById("stat-products-count");

  // Elements - Orders Table
  const adminOrdersTbody = document.getElementById("admin-orders-tbody");
  const btnRefreshAdminOrders = document.getElementById("btn-refresh-admin-orders");

  // Elements - Categories
  const adminCategoriesTbody = document.getElementById("admin-categories-tbody");
  const btnRefreshAdminCategories = document.getElementById("btn-refresh-admin-categories");
  const btnOpenAddCategory = document.getElementById("btn-open-add-category");
  const modalCategory = document.getElementById("modal-category");
  const modalCategoryTitle = document.getElementById("modal-category-title");
  const formCategory = document.getElementById("form-category");
  const categoryEditId = document.getElementById("category-edit-id");
  const categoryTypeSelect = document.getElementById("category-type");
  const categoryNameInput = document.getElementById("category-name");
  const btnCloseCategoryModal = document.getElementById("btn-close-category-modal");
  const btnCancelCategory = document.getElementById("btn-cancel-category");

  // Elements - Products
  const adminProductsTbody = document.getElementById("admin-products-tbody");
  const btnRefreshAdminProducts = document.getElementById("btn-refresh-admin-products");
  const btnOpenAddProduct = document.getElementById("btn-open-add-product");
  const modalAddProduct = document.getElementById("modal-add-product");
  const modalProductTitle = document.getElementById("modal-product-title");
  const btnCloseProductModal = document.getElementById("btn-close-product-modal");
  const btnCancelAddProduct = document.getElementById("btn-cancel-add-product");
  const formAddProduct = document.getElementById("form-add-product");
  const prodEditId = document.getElementById("prod-edit-id");
  const prodTypeSelect = document.getElementById("prod-type");
  const prodCategorySelect = document.getElementById("prod-category");
  const wrapAuthor = document.getElementById("wrap-author");
  const wrapBrand = document.getElementById("wrap-brand");
  const wrapBookExtra = document.getElementById("wrap-book-extra");

  // Elements - Image Upload
  const prodImageFile = document.getElementById("prod-image-file");
  const prodImageUrl = document.getElementById("prod-image-url");
  const imagePlaceholder = document.getElementById("image-placeholder");
  const imagePreviewImg = document.getElementById("image-preview-img");
  const btnChooseImage = document.getElementById("btn-choose-image");
  const btnRemoveImage = document.getElementById("btn-remove-image");
  const imageUploadStatus = document.getElementById("image-upload-status");

  // Elements - Change Password Modal
  const modalChangePassword = document.getElementById("modal-change-password");
  const btnCloseChangePwModal = document.getElementById("btn-close-change-pw-modal");
  const btnCancelChangePw = document.getElementById("btn-cancel-change-pw");
  const formChangePassword = document.getElementById("form-change-password");
  const changePwAlert = document.getElementById("change-pw-alert");

  const toast = document.getElementById("toast");

  // Cache
  let cachedProducts = [];
  let cachedCategories = [];

  const money = (n) => "৳" + Number(n || 0).toLocaleString("en-BD");

  function showToast(message) {
    if (!toast) return;
    toast.hidden = false;
    toast.textContent = message;
    window.clearTimeout(showToast.timer);
    showToast.timer = window.setTimeout(() => {
      toast.hidden = true;
    }, 2400);
  }

  function escapeHtml(str) {
    if (!str) return "";
    return String(str)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }

  /* === AUTHENTICATION & SETUP INITIALIZATION === */
  async function initAdminAuth() {
    try {
      const statusResp = await fetch("/api/admin/setup-status");
      const statusData = await statusResp.json();

      if (!statusData.setup_completed) {
        // First-Time Setup Required
        showSetupView();
        return;
      }

      // Setup is completed - Check if current session is authenticated as admin
      const meResp = await fetch("/api/auth/me");
      if (meResp.ok) {
        const meData = await meResp.json();
        if (meData.authenticated && meData.role === "admin") {
          showDashboardView();
          return;
        }
      }

      // Not authenticated - Show login card
      showLoginView();
    } catch (err) {
      console.error("Auth init error:", err);
      showLoginView();
    }
  }

  function showSetupView() {
    adminAuthView.style.display = "block";
    cardAdminSetup.style.display = "block";
    cardAdminLogin.style.display = "none";
    adminDashboardContainer.style.display = "none";
    adminUserPill.style.display = "none";
    if (btnOpenChangePw) btnOpenChangePw.style.display = "none";
    if (adminLogoutBtn) adminLogoutBtn.style.display = "none";
  }

  function showLoginView() {
    adminAuthView.style.display = "block";
    cardAdminSetup.style.display = "none";
    cardAdminLogin.style.display = "block";
    adminDashboardContainer.style.display = "none";
    adminUserPill.style.display = "none";
    if (btnOpenChangePw) btnOpenChangePw.style.display = "none";
    if (adminLogoutBtn) adminLogoutBtn.style.display = "none";
  }

  function showDashboardView() {
    adminAuthView.style.display = "none";
    adminDashboardContainer.style.display = "block";
    adminUserPill.style.display = "inline-block";
    if (btnOpenChangePw) btnOpenChangePw.style.display = "inline-block";
    if (adminLogoutBtn) adminLogoutBtn.style.display = "inline-block";

    // Load initial data
    loadAdminOrders();
    loadAdminCategories();
    loadAdminProducts();
  }

  /* === FORM: FIRST-TIME SETUP === */
  if (formAdminSetup) {
    formAdminSetup.addEventListener("submit", async (e) => {
      e.preventDefault();
      setupAlert.style.display = "none";

      const username = document.getElementById("setup-username").value.trim();
      const password = document.getElementById("setup-password").value;
      const confirmPassword = document.getElementById("setup-confirm-password").value;

      if (!username || !password) {
        setupAlert.textContent = "Please fill in all fields.";
        setupAlert.style.display = "flex";
        return;
      }

      if (password.length < 6) {
        setupAlert.textContent = "Password must be at least 6 characters long.";
        setupAlert.style.display = "flex";
        return;
      }

      if (password !== confirmPassword) {
        setupAlert.textContent = "Passwords do not match.";
        setupAlert.style.display = "flex";
        return;
      }

      try {
        const resp = await fetch("/api/admin/setup", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ username, password }),
        });

        const data = await resp.json();
        if (resp.ok && data.status === "success") {
          showToast("Admin account configured successfully!");
          showDashboardView();
        } else {
          setupAlert.textContent = data.message || "Admin setup failed.";
          setupAlert.style.display = "flex";
        }
      } catch (err) {
        setupAlert.textContent = "Network error: " + err.message;
        setupAlert.style.display = "flex";
      }
    });
  }

  /* === FORM: ADMIN LOGIN === */
  if (formAdminLogin) {
    formAdminLogin.addEventListener("submit", async (e) => {
      e.preventDefault();
      loginAlert.style.display = "none";

      const username = document.getElementById("login-username").value.trim();
      const password = document.getElementById("login-password").value;

      if (!username || !password) {
        loginAlert.textContent = "Please enter username and password.";
        loginAlert.style.display = "flex";
        return;
      }

      try {
        const resp = await fetch("/api/admin/login", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ username, password }),
        });

        const data = await resp.json();
        if (resp.ok && data.status === "success") {
          showToast("Signed in as Admin.");
          showDashboardView();
        } else {
          loginAlert.textContent = data.message || "Invalid username or password.";
          loginAlert.style.display = "flex";
        }
      } catch (err) {
        loginAlert.textContent = "Network error: " + err.message;
        loginAlert.style.display = "flex";
      }
    });
  }

  /* === ADMIN LOGOUT === */
  if (adminLogoutBtn) {
    adminLogoutBtn.addEventListener("click", async () => {
      try {
        await fetch("/api/auth/logout", { method: "POST" });
        showToast("Logged out successfully.");
        showLoginView();
      } catch (err) {
        showLoginView();
      }
    });
  }

  /* === CHANGE PASSWORD MODAL === */
  if (btnOpenChangePw && modalChangePassword) {
    btnOpenChangePw.addEventListener("click", () => {
      if (formChangePassword) formChangePassword.reset();
      if (changePwAlert) changePwAlert.style.display = "none";
      modalChangePassword.showModal();
    });
  }

  function closeChangePwModal() {
    if (modalChangePassword && modalChangePassword.open) {
      modalChangePassword.close();
    }
  }

  if (btnCloseChangePwModal) btnCloseChangePwModal.addEventListener("click", closeChangePwModal);
  if (btnCancelChangePw) btnCancelChangePw.addEventListener("click", closeChangePwModal);

  if (formChangePassword) {
    formChangePassword.addEventListener("submit", async (e) => {
      e.preventDefault();
      if (changePwAlert) changePwAlert.style.display = "none";

      const curPw = document.getElementById("pw-current").value;
      const newPw = document.getElementById("pw-new").value;
      const confPw = document.getElementById("pw-confirm").value;

      if (!curPw || !newPw) {
        changePwAlert.textContent = "Please fill in all fields.";
        changePwAlert.style.display = "flex";
        return;
      }

      if (newPw.length < 6) {
        changePwAlert.textContent = "New password must be at least 6 characters long.";
        changePwAlert.style.display = "flex";
        return;
      }

      if (newPw !== confPw) {
        changePwAlert.textContent = "New passwords do not match.";
        changePwAlert.style.display = "flex";
        return;
      }

      try {
        const resp = await fetch("/api/admin/change-password", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ current_password: curPw, new_password: newPw }),
        });

        const data = await resp.json();
        if (resp.ok && data.status === "success") {
          showToast("Password updated successfully!");
          closeChangePwModal();
        } else {
          changePwAlert.textContent = data.message || "Failed to update password.";
          changePwAlert.style.display = "flex";
        }
      } catch (err) {
        changePwAlert.textContent = "Network error: " + err.message;
        changePwAlert.style.display = "flex";
      }
    });
  }

  /* === CATEGORY MANAGEMENT === */
  async function loadAdminCategories() {
    try {
      const resp = await fetch("/api/categories");
      if (!resp.ok) {
        adminCategoriesTbody.innerHTML = `<tr><td colspan="4">Failed to load categories.</td></tr>`;
        return;
      }

      const cats = await resp.json();
      cachedCategories = cats;

      // Populate Category dropdown in Add/Edit Product modal
      if (prodCategorySelect) {
        const currentSelected = prodCategorySelect.value;
        prodCategorySelect.innerHTML = cats
          .map((c) => `<option value="${escapeHtml(c.name)}">${escapeHtml(c.name)} (${c.type})</option>`)
          .join("");
        if (currentSelected) prodCategorySelect.value = currentSelected;
      }

      if (!cats.length) {
        adminCategoriesTbody.innerHTML = `<tr><td colspan="4">No categories defined yet.</td></tr>`;
        return;
      }

      adminCategoriesTbody.innerHTML = cats
        .map((c) => {
          const typeBadgeClass = c.type === "stationery" ? "cat-type-stationery" : "cat-type-book";
          const count = c.product_count || 0;
          return `
            <tr>
              <td><strong>${escapeHtml(c.name)}</strong></td>
              <td><span class="cat-type-badge ${typeBadgeClass}">${escapeHtml(c.type)}</span></td>
              <td><span class="cat-count-pill">${count} active item${count === 1 ? '' : 's'}</span></td>
              <td>
                <div style="display:flex;gap:6px;">
                  <button class="btn-xs btn-outline btn-edit-category" data-cat-id="${c.id}">Edit</button>
                  <button class="btn-xs btn-danger-outline btn-delete-category" data-cat-id="${c.id}" data-cat-name="${escapeHtml(c.name)}" data-prod-count="${count}">Delete</button>
                </div>
              </td>
            </tr>
          `;
        })
        .join("");

      // Bind Edit Category buttons
      document.querySelectorAll(".btn-edit-category").forEach((btn) => {
        btn.addEventListener("click", () => {
          const id = Number(btn.getAttribute("data-cat-id"));
          const cat = cachedCategories.find((c) => c.id === id);
          if (!cat) return;

          categoryEditId.value = String(cat.id);
          categoryTypeSelect.value = cat.type;
          categoryNameInput.value = cat.name;
          modalCategoryTitle.textContent = "Edit Category";
          modalCategory.showModal();
        });
      });

      // Bind Delete Category buttons
      document.querySelectorAll(".btn-delete-category").forEach((btn) => {
        btn.addEventListener("click", async () => {
          const id = btn.getAttribute("data-cat-id");
          const name = btn.getAttribute("data-cat-name");
          const count = Number(btn.getAttribute("data-prod-count") || 0);

          if (count > 0) {
            alert(`Cannot delete category "${name}" because it contains ${count} active product(s). Please reassign or delete the products first.`);
            return;
          }

          if (!confirm(`Are you sure you want to delete category "${name}"?`)) {
            return;
          }

          try {
            const delResp = await fetch(`/api/admin/categories/${id}`, { method: "DELETE" });
            const data = await delResp.json();
            if (delResp.ok && data.status === "success") {
              showToast(`Category "${name}" deleted.`);
              loadAdminCategories();
            } else {
              alert(data.message || "Failed to delete category.");
            }
          } catch (err) {
            showToast("Network error: " + err.message);
          }
        });
      });
    } catch (err) {
      console.warn("Category load error:", err);
      adminCategoriesTbody.innerHTML = `<tr><td colspan="4">Error: ${err.message}</td></tr>`;
    }
  }

  if (btnOpenAddCategory && modalCategory) {
    btnOpenAddCategory.addEventListener("click", () => {
      formCategory.reset();
      categoryEditId.value = "";
      modalCategoryTitle.textContent = "Add New Category";
      modalCategory.showModal();
    });
  }

  function closeCategoryModal() {
    if (modalCategory && modalCategory.open) {
      modalCategory.close();
    }
  }

  if (btnCloseCategoryModal) btnCloseCategoryModal.addEventListener("click", closeCategoryModal);
  if (btnCancelCategory) btnCancelCategory.addEventListener("click", closeCategoryModal);

  if (formCategory) {
    formCategory.addEventListener("submit", async (e) => {
      e.preventDefault();
      const id = categoryEditId.value;
      const type = categoryTypeSelect.value;
      const name = categoryNameInput.value.trim();

      if (!name) {
        showToast("Please enter a category name.");
        return;
      }

      try {
        let resp;
        if (id) {
          resp = await fetch(`/api/admin/categories/${id}`, {
            method: "PUT",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ name, type }),
          });
        } else {
          resp = await fetch("/api/admin/categories", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ name, type }),
          });
        }

        const data = await resp.json();
        if (resp.ok && data.status === "success") {
          showToast(id ? "Category updated." : "Category created.");
          closeCategoryModal();
          loadAdminCategories();
          loadAdminProducts();
        } else {
          showToast(data.message || "Failed to save category.");
        }
      } catch (err) {
        showToast("Network error: " + err.message);
      }
    });
  }

  if (btnRefreshAdminCategories) {
    btnRefreshAdminCategories.addEventListener("click", () => {
      loadAdminCategories();
      showToast("Categories refreshed.");
    });
  }

  /* === PRODUCT MODAL & IMAGE UPLOAD LOGIC === */
  function updateFormTypeFields() {
    if (!prodTypeSelect) return;
    const isBook = prodTypeSelect.value === "book";
    if (wrapAuthor) wrapAuthor.style.display = isBook ? "block" : "none";
    if (wrapBrand) wrapBrand.style.display = isBook ? "none" : "block";
    if (wrapBookExtra) wrapBookExtra.style.display = isBook ? "flex" : "none";
  }

  if (prodTypeSelect) {
    prodTypeSelect.addEventListener("change", updateFormTypeFields);
  }

  function setImagePreview(url) {
    prodImageUrl.value = url || "";
    if (url) {
      imagePreviewImg.src = url;
      imagePreviewImg.style.display = "block";
      imagePlaceholder.style.display = "none";
      btnRemoveImage.style.display = "inline-flex";
    } else {
      imagePreviewImg.src = "";
      imagePreviewImg.style.display = "none";
      imagePlaceholder.style.display = "flex";
      btnRemoveImage.style.display = "none";
    }
  }

  if (btnChooseImage && prodImageFile) {
    btnChooseImage.addEventListener("click", () => {
      prodImageFile.click();
    });
  }

  if (btnRemoveImage) {
    btnRemoveImage.addEventListener("click", () => {
      setImagePreview("");
      if (prodImageFile) prodImageFile.value = "";
      if (imageUploadStatus) imageUploadStatus.textContent = "Photo removed.";
    });
  }

  if (prodImageFile) {
    prodImageFile.addEventListener("change", () => {
      const file = prodImageFile.files[0];
      if (!file) return;

      // Validate size <= 5MB
      if (file.size > 5 * 1024 * 1024) {
        showToast("Image size exceeds 5MB limit.");
        prodImageFile.value = "";
        return;
      }

      if (imageUploadStatus) imageUploadStatus.textContent = "Uploading to C backend...";

      const reader = new FileReader();
      reader.onload = async function (e) {
        const base64Data = e.target.result;
        // Instant visual preview
        setImagePreview(base64Data);

        try {
          const resp = await fetch("/api/admin/upload", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ data: base64Data }),
          });

          const data = await resp.json();
          if (resp.ok && data.status === "success" && data.url) {
            setImagePreview(data.url);
            if (imageUploadStatus) imageUploadStatus.textContent = "Photo saved to disk.";
            showToast("Photo uploaded successfully.");
          } else {
            setImagePreview("");
            if (imageUploadStatus) imageUploadStatus.textContent = "Upload failed: " + (data.message || "Invalid format");
            showToast(data.message || "Failed to upload image.");
          }
        } catch (err) {
          if (imageUploadStatus) imageUploadStatus.textContent = "Upload error: " + err.message;
          showToast("Network error during image upload.");
        }
      };
      reader.readAsDataURL(file);
    });
  }

  function closeProductModal() {
    if (modalAddProduct && modalAddProduct.open) {
      modalAddProduct.close();
    }
  }

  if (btnCloseProductModal) btnCloseProductModal.addEventListener("click", closeProductModal);
  if (btnCancelAddProduct) btnCancelAddProduct.addEventListener("click", closeProductModal);

  if (btnOpenAddProduct && modalAddProduct) {
    btnOpenAddProduct.addEventListener("click", () => {
      if (formAddProduct) formAddProduct.reset();
      prodEditId.value = "";
      modalProductTitle.textContent = "Add New Product";
      setImagePreview("");
      if (imageUploadStatus) imageUploadStatus.textContent = "";
      updateFormTypeFields();
      modalAddProduct.showModal();
    });
  }

  /* === PRODUCT FORM SUBMISSION (ADD OR EDIT) === */
  if (formAddProduct) {
    formAddProduct.addEventListener("submit", async (e) => {
      e.preventDefault();

      const editId = prodEditId.value;
      const type = prodTypeSelect ? prodTypeSelect.value : "book";
      const title = document.getElementById("prod-title").value.trim();
      const sku = document.getElementById("prod-sku").value.trim();
      const category = prodCategorySelect.value || document.getElementById("prod-category").value.trim();
      const price = parseFloat(document.getElementById("prod-price").value);
      const stock_quantity = parseInt(document.getElementById("prod-stock").value, 10);
      const description = document.getElementById("prod-description").value.trim();
      const image_url = prodImageUrl.value.trim();

      if (!title || !category || isNaN(price) || price <= 0) {
        showToast("Please provide a valid title, category, and positive price.");
        return;
      }

      const payload = {
        type,
        title,
        category,
        price,
        stock_quantity: isNaN(stock_quantity) ? 10 : stock_quantity,
        description,
        image_url,
      };

      if (sku) payload.sku = sku;

      if (type === "book") {
        const author = document.getElementById("prod-author").value.trim();
        const isbn = document.getElementById("prod-isbn").value.trim();
        const edition = document.getElementById("prod-edition").value.trim();
        if (author) payload.author = author;
        if (isbn) payload.isbn = isbn;
        if (edition) payload.edition = edition;
      } else {
        const brand = document.getElementById("prod-brand").value.trim();
        if (brand) payload.brand = brand;
      }

      try {
        let resp;
        if (editId) {
          resp = await fetch(`/api/admin/products/${editId}`, {
            method: "PUT",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(payload),
          });
        } else {
          resp = await fetch("/api/admin/products", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(payload),
          });
        }

        const data = await resp.json();
        if (resp.ok && data.status === "success") {
          showToast(editId ? `Product updated (${title})` : `Product created (${data.sku || title})`);
          closeProductModal();
          formAddProduct.reset();
          loadAdminProducts();
          loadAdminCategories();
        } else {
          showToast(data.message || "Failed to save product.");
        }
      } catch (err) {
        showToast("Network error: " + err.message);
      }
    });
  }

  /* === LOAD PRODUCTS CATALOG === */
  async function loadAdminProducts() {
    try {
      const resp = await fetch("/api/admin/products");
      if (!resp.ok) {
        adminProductsTbody.innerHTML = `<tr><td colspan="9">Failed to fetch catalog from C backend.</td></tr>`;
        return;
      }

      const products = await resp.json();
      cachedProducts = products;

      if (statProductsCount) {
        statProductsCount.textContent = String(products.length);
      }

      if (!products.length) {
        adminProductsTbody.innerHTML = `<tr><td colspan="9">No products found in database.</td></tr>`;
        return;
      }

      adminProductsTbody.innerHTML = products
        .map((p) => {
          const typeBadge = p.type === "book" ? "cat-type-book" : "cat-type-stationery";
          const statusBadge = p.is_active
            ? `<span class="badge" style="background:rgba(47,91,76,0.15);color:var(--forest);padding:2px 8px;border-radius:6px;font-size:0.78rem;font-weight:700;">Active</span>`
            : `<span class="badge" style="background:rgba(220,53,69,0.15);color:#c93b48;padding:2px 8px;border-radius:6px;font-size:0.78rem;font-weight:700;">Disabled</span>`;

          const toggleBtnText = p.is_active ? "Deactivate" : "Activate";
          const toggleBtnClass = p.is_active ? "btn-danger-outline" : "btn-success-outline";

          const photoHtml = p.image_url
            ? `<img src="${escapeHtml(p.image_url)}" alt="${escapeHtml(p.title)}" style="width:38px;height:38px;object-fit:cover;border-radius:6px;border:1px solid var(--line);" />`
            : `<span style="display:inline-block;width:38px;height:38px;background:var(--paper);border:1px solid var(--line);border-radius:6px;line-height:36px;text-align:center;font-size:0.75rem;color:var(--ink-soft);">Art</span>`;

          return `
            <tr>
              <td><code>${escapeHtml(p.sku)}</code></td>
              <td style="text-align:center;">${photoHtml}</td>
              <td><strong>${escapeHtml(p.title)}</strong></td>
              <td><span class="cat-type-badge ${typeBadge}">${escapeHtml(p.type)}</span></td>
              <td>${escapeHtml(p.category)}</td>
              <td>${money(p.price)}</td>
              <td>
                <div class="inline-stock-wrap">
                  <input type="number" min="0" class="stock-inline-input" value="${p.stock_quantity}" data-prod-id="${p.id}" />
                  <button class="btn-xs btn-outline btn-save-stock" data-prod-id="${p.id}" title="Save Stock">✓</button>
                </div>
              </td>
              <td>${statusBadge}</td>
              <td>
                <div style="display:flex;gap:6px;">
                  <button class="btn-xs btn-outline btn-edit-prod" data-prod-id="${p.id}">Edit</button>
                  <button class="btn-xs ${toggleBtnClass} btn-toggle-active" data-prod-id="${p.id}" data-active="${p.is_active}">
                    ${toggleBtnText}
                  </button>
                </div>
              </td>
            </tr>
          `;
        })
        .join("");

      // Bind Inline Stock Saving
      document.querySelectorAll(".btn-save-stock").forEach((btn) => {
        btn.addEventListener("click", async () => {
          const prodId = btn.getAttribute("data-prod-id");
          const input = document.querySelector(`.stock-inline-input[data-prod-id="${prodId}"]`);
          const newStock = parseInt(input.value, 10);

          if (isNaN(newStock) || newStock < 0) {
            showToast("Stock quantity must be a non-negative integer.");
            return;
          }

          try {
            const patchResp = await fetch(`/api/admin/products/${prodId}`, {
              method: "PATCH",
              headers: { "Content-Type": "application/json" },
              body: JSON.stringify({ stock_quantity: newStock }),
            });
            const data = await patchResp.json();
            if (patchResp.ok && data.status === "success") {
              showToast(`Stock updated to ${newStock} units`);
              loadAdminProducts();
            } else {
              showToast(data.message || "Failed to update stock.");
            }
          } catch (err) {
            showToast("Network error: " + err.message);
          }
        });
      });

      // Enter key in stock input
      document.querySelectorAll(".stock-inline-input").forEach((input) => {
        input.addEventListener("keydown", (e) => {
          if (e.key === "Enter") {
            e.preventDefault();
            const prodId = input.getAttribute("data-prod-id");
            const btn = document.querySelector(`.btn-save-stock[data-prod-id="${prodId}"]`);
            if (btn) btn.click();
          }
        });
      });

      // Bind Edit Product buttons
      document.querySelectorAll(".btn-edit-prod").forEach((btn) => {
        btn.addEventListener("click", () => {
          const id = Number(btn.getAttribute("data-prod-id"));
          const p = cachedProducts.find((item) => item.id === id);
          if (!p) return;

          prodEditId.value = String(p.id);
          modalProductTitle.textContent = "Edit Product";
          if (prodTypeSelect) prodTypeSelect.value = p.type;
          document.getElementById("prod-sku").value = p.sku || "";
          document.getElementById("prod-title").value = p.title || "";
          document.getElementById("prod-price").value = p.price || "";
          document.getElementById("prod-stock").value = p.stock_quantity;
          document.getElementById("prod-description").value = p.description || "";

          if (p.type === "book") {
            document.getElementById("prod-author").value = p.author || "";
            document.getElementById("prod-isbn").value = p.isbn || "";
            document.getElementById("prod-edition").value = p.edition || "";
          } else {
            document.getElementById("prod-brand").value = p.brand || "";
          }

          if (prodCategorySelect) prodCategorySelect.value = p.category;

          setImagePreview(p.image_url || "");
          if (imageUploadStatus) imageUploadStatus.textContent = "";

          updateFormTypeFields();
          modalAddProduct.showModal();
        });
      });

      // Bind Deactivation / Activation Toggle
      document.querySelectorAll(".btn-toggle-active").forEach((btn) => {
        btn.addEventListener("click", async () => {
          const prodId = btn.getAttribute("data-prod-id");
          const currentActive = btn.getAttribute("data-active") === "1";

          try {
            let res;
            if (currentActive) {
              res = await fetch(`/api/admin/products/${prodId}`, { method: "DELETE" });
            } else {
              res = await fetch(`/api/admin/products/${prodId}`, {
                method: "PATCH",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ is_active: 1 }),
              });
            }

            const data = await res.json();
            if (res.ok && data.status === "success") {
              showToast(data.message || (currentActive ? "Product deactivated" : "Product reactivated"));
              loadAdminProducts();
            } else {
              showToast(data.message || "Failed to update product status.");
            }
          } catch (err) {
            showToast("Network error: " + err.message);
          }
        });
      });
    } catch (err) {
      console.warn("Product load error:", err);
      adminProductsTbody.innerHTML = `<tr><td colspan="9">Error: ${err.message}</td></tr>`;
    }
  }

  if (btnRefreshAdminProducts) {
    btnRefreshAdminProducts.addEventListener("click", () => {
      loadAdminProducts();
      showToast("Products refreshed.");
    });
  }

  /* === LOAD ORDERS === */
  async function loadAdminOrders() {
    try {
      const resp = await fetch("/api/admin/orders");
      if (!resp.ok) {
        adminOrdersTbody.innerHTML = `<tr><td colspan="8">Failed to fetch orders from C backend.</td></tr>`;
        return;
      }

      const orders = await resp.json();
      if (!Array.isArray(orders) || !orders.length) {
        adminOrdersTbody.innerHTML = `<tr><td colspan="8">No customer orders placed yet.</td></tr>`;
        if (statOrdersCount) statOrdersCount.textContent = "0";
        if (statRevenue) statRevenue.textContent = money(0);
        return;
      }

      if (statOrdersCount) statOrdersCount.textContent = String(orders.length);

      const totalRev = orders.reduce((sum, o) => {
        return (o.status !== "Cancelled") ? sum + (Number(o.total_amount) || 0) : sum;
      }, 0);
      if (statRevenue) statRevenue.textContent = money(totalRev);

      const statusOptions = ["Pending", "Confirmed", "Shipped", "Delivered", "Cancelled"];

      adminOrdersTbody.innerHTML = orders
        .map((o) => {
          const selectHtml = `
            <select class="admin-order-status-select" data-order-id="${o.id}">
              ${statusOptions
                .map((opt) => `<option value="${opt}" ${o.status === opt ? "selected" : ""}>${opt}</option>`)
                .join("")}
            </select>
          `;

          return `
            <tr>
              <td><strong>#${o.id}</strong></td>
              <td>${escapeHtml(o.customer_name || "Guest")}</td>
              <td>${escapeHtml(o.phone || "-")}</td>
              <td style="max-width:180px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;" title="${escapeHtml(o.shipping_address)}">
                ${escapeHtml(o.shipping_address || "-")}
              </td>
              <td><strong>${money(o.total_amount)}</strong></td>
              <td>${escapeHtml(o.payment_method || "COD")}</td>
              <td><span class="status-badge status-${(o.status || 'pending').toLowerCase()}">${escapeHtml(o.status)}</span></td>
              <td>${selectHtml}</td>
            </tr>
          `;
        })
        .join("");

      document.querySelectorAll(".admin-order-status-select").forEach((sel) => {
        sel.addEventListener("change", async () => {
          const orderId = sel.getAttribute("data-order-id");
          const newStatus = sel.value;

          try {
            const patchResp = await fetch(`/api/admin/orders/${orderId}`, {
              method: "PATCH",
              headers: { "Content-Type": "application/json" },
              body: JSON.stringify({ status: newStatus }),
            });
            const data = await patchResp.json();
            if (patchResp.ok && data.status === "success") {
              showToast(`Order #${orderId} marked as ${newStatus}`);
              loadAdminOrders();
            } else {
              showToast("Failed to update order status.");
            }
          } catch (err) {
            showToast("Network error: " + err.message);
          }
        });
      });
    } catch (err) {
      console.warn("Orders error:", err);
      adminOrdersTbody.innerHTML = `<tr><td colspan="8">Error loading orders: ${err.message}</td></tr>`;
    }
  }

  if (btnRefreshAdminOrders) {
    btnRefreshAdminOrders.addEventListener("click", () => {
      loadAdminOrders();
      showToast("Orders refreshed.");
    });
  }

  // Kick off auth check on page load
  initAdminAuth();
})();
