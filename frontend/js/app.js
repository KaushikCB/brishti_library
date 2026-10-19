(function () {
  let products = [
    {
      id: 1,
      sku: "B001",
      type: "book",
      title: "Monsoon Algorithms",
      meta: "N. Rahman · CS",
      price: 890,
      stock_quantity: 24,
      cover_class: "cover-book-1",
      description: "Sorting storms, graphs, and exam week. Essential for CSE214.",
    },
    {
      id: 2,
      sku: "B002",
      type: "book",
      title: "The Quiet Stack",
      meta: "A. Chowdhury · Fiction",
      price: 650,
      stock_quantity: 18,
      cover_class: "cover-book-2",
      description: "A campus novel about libraries after hours.",
    },
    {
      id: 3,
      sku: "B003",
      type: "book",
      title: "Paper Weather",
      meta: "L. Ahmed · Essays",
      price: 720,
      stock_quantity: 12,
      cover_class: "cover-book-3",
      description: "Short essays on reading in the rain.",
    },
    {
      id: 4,
      sku: "B004",
      type: "book",
      title: "Graph of Home",
      meta: "S. Islam · CS",
      price: 980,
      stock_quantity: 15,
      cover_class: "cover-book-4",
      description: "Paths, trees, and finding your way back.",
    },
    {
      id: 5,
      sku: "S001",
      type: "stationery",
      title: "Rain Hall Notebook",
      meta: "A5 · 120 pages",
      price: 220,
      stock_quantity: 38,
      cover_class: "cover-stat-1",
      description: "Dot grid paper for lectures and labs.",
    },
    {
      id: 6,
      sku: "S002",
      type: "stationery",
      title: "Forest Ink Pen",
      meta: "Fine nib",
      price: 340,
      stock_quantity: 35,
      cover_class: "cover-stat-2",
      description: "Smooth gel ink for long problem sets.",
    },
    {
      id: 7,
      sku: "S003",
      type: "stationery",
      title: "Goldclip Highlighters",
      meta: "Set of 4",
      price: 180,
      stock_quantity: 30,
      cover_class: "cover-stat-3",
      description: "Soft tones that do not bleed through.",
    },
    {
      id: 8,
      sku: "S004",
      type: "stationery",
      title: "Desk Storm Sticky Notes",
      meta: "3 packs",
      price: 150,
      stock_quantity: 50,
      cover_class: "cover-stat-4",
      description: "Reminders for due dates and page marks.",
    },
  ];

  let currentUser = null;
  const money = (n) => "৳" + Number(n || 0).toLocaleString("en-BD");

  const booksGrid = document.getElementById("books-grid");
  const stationeryGrid = document.getElementById("stationery-grid");
  const searchForm = document.getElementById("search-form");
  const searchInput = document.getElementById("search-input");
  const searchStatus = document.getElementById("search-status");

  const cartOpen = document.getElementById("cart-open");
  const cartClose = document.getElementById("cart-close");
  const cartDrawer = document.getElementById("cart-drawer");
  const cartItems = document.getElementById("cart-items");
  const cartCount = document.getElementById("cart-count");
  const cartSubtotal = document.getElementById("cart-subtotal");
  const cartDelivery = document.getElementById("cart-delivery");
  const cartTotal = document.getElementById("cart-total");

  const menuToggle = document.getElementById("menu-toggle");
  const primaryNav = document.getElementById("primary-nav");
  const toast = document.getElementById("toast");

  /* Checkout elements */
  const checkoutOpenBtn = document.getElementById("checkout-open-btn");
  const checkoutModal = document.getElementById("checkout-modal");
  const checkoutCloseBtn = document.getElementById("checkout-close-btn");
  const checkoutCancelBtn = document.getElementById("checkout-cancel-btn");
  const checkoutForm = document.getElementById("checkout-form");
  const checkoutTotalVal = document.getElementById("checkout-total-val");

  /* Auth elements */
  const authToggle = document.getElementById("auth-toggle");
  const authModal = document.getElementById("auth-modal");
  const authCloseBtn = document.getElementById("auth-close-btn");
  const demoStudentBtn = document.getElementById("demo-student-btn");
  const demoAdminBtn = document.getElementById("demo-admin-btn");

  /* Orders elements */
  const ordersList = document.getElementById("orders-list");
  const refreshOrdersBtn = document.getElementById("refresh-orders-btn");

  /* Catalog Filter & Sort Elements */
  const filterCategory = document.getElementById("filter-category");
  const filterSort = document.getElementById("filter-sort");
  const filterInStock = document.getElementById("filter-instock");
  const btnResetFilters = document.getElementById("btn-reset-filters");
  const catalogCountBadge = document.getElementById("catalog-count-badge");
  const activeFilterTags = document.getElementById("active-filter-tags");

  /* Product Detail Modal Elements */
  const productDetailModal = document.getElementById("product-detail-modal");
  const detailModalCloseBtn = document.getElementById("detail-modal-close-btn");
  const detailProductType = document.getElementById("detail-product-type");
  const detailProductTitle = document.getElementById("detail-product-title");
  const detailCoverBox = document.getElementById("detail-cover-box");
  const detailCoverTitle = document.getElementById("detail-cover-title");
  const detailSku = document.getElementById("detail-sku");
  const detailStockBadge = document.getElementById("detail-stock-badge");
  const detailPrice = document.getElementById("detail-price");
  const detailDiscount = document.getElementById("detail-discount");
  const detailDescription = document.getElementById("detail-description");
  const detailMetaList = document.getElementById("detail-meta-list");
  const detailQtyInput = document.getElementById("detail-qty-input");
  const detailQtyDec = document.getElementById("detail-qty-dec");
  const detailQtyInc = document.getElementById("detail-qty-inc");
  const detailAddCartBtn = document.getElementById("detail-add-cart-btn");

  let currentDetailProduct = null;

  let activeFilters = {
    category: "all",
    sort: "featured",
    inStock: false,
    search: "",
  };

  const CART_KEY = "brishtilibrary-cart";

  function readCart() {
    try {
      const data = JSON.parse(localStorage.getItem(CART_KEY)) || [];
      return data.map((item) => ({
        id: Number(item.id) || item.id,
        qty: Math.max(1, Number(item.qty) || 1),
      }));
    } catch (err) {
      return [];
    }
  }

  function writeCart(cart) {
    localStorage.setItem(CART_KEY, JSON.stringify(cart));
  }

  function showToast(message) {
    if (!toast) return;
    toast.hidden = false;
    toast.textContent = message;
    window.clearTimeout(showToast.timer);
    showToast.timer = window.setTimeout(() => {
      toast.hidden = true;
    }, 2200);
  }

  /* Fetch categories from SQLite C backend */
  async function loadCategories() {
    if (!filterCategory) return;
    try {
      const resp = await fetch("/api/categories");
      if (resp.ok) {
        const cats = await resp.json();
        if (Array.isArray(cats)) {
          const names = Array.from(new Set(cats.map((c) => c.name))).sort();
          filterCategory.innerHTML =
            '<option value="all">All Categories</option>' +
            names
              .map((name) => `<option value="${escapeHtml(name)}">${escapeHtml(name)}</option>`)
              .join("");
          if (activeFilters.category) {
            filterCategory.value = activeFilters.category;
          }
        }
      }
    } catch (e) {
      console.warn("Could not load categories:", e);
    }
  }

  /* Fetch products from SQLite C backend with filter/sort/search params */
  async function fetchProducts() {
    try {
      const params = new URLSearchParams();
      if (activeFilters.category && activeFilters.category !== "all") {
        params.append("category", activeFilters.category);
      }
      if (activeFilters.search) {
        params.append("q", activeFilters.search);
      }
      if (activeFilters.sort && activeFilters.sort !== "featured") {
        params.append("sort", activeFilters.sort);
      }
      if (activeFilters.inStock) {
        params.append("availability", "in_stock");
      }

      const queryString = params.toString();
      const url = queryString ? `/api/products?${queryString}` : "/api/products";
      const resp = await fetch(url);
      if (resp.ok) {
        const data = await resp.json();
        if (Array.isArray(data)) {
          products = data;
        }
      }
    } catch (err) {
      console.warn("Using offline fallback catalog:", err);
    }

    renderCatalog();
    updateFilterUI();
  }

  function updateFilterUI() {
    if (catalogCountBadge) {
      catalogCountBadge.textContent = `${products.length} product(s) available`;
    }

    if (searchStatus) {
      if (activeFilters.search) {
        searchStatus.hidden = false;
        searchStatus.innerHTML = `Showing results for <strong>"${escapeHtml(activeFilters.search)}"</strong> (${products.length} found)`;
      } else {
        searchStatus.hidden = true;
      }
    }

    if (activeFilterTags) {
      const tags = [];
      if (activeFilters.category !== "all") {
        tags.push(`
          <span class="filter-tag">
            Category: ${escapeHtml(activeFilters.category)}
            <button type="button" data-remove-filter="category" title="Remove category filter">&times;</button>
          </span>
        `);
      }
      if (activeFilters.inStock) {
        tags.push(`
          <span class="filter-tag">
            In Stock Only
            <button type="button" data-remove-filter="instock" title="Show all items">&times;</button>
          </span>
        `);
      }
      if (activeFilters.search) {
        tags.push(`
          <span class="filter-tag">
            Search: "${escapeHtml(activeFilters.search)}"
            <button type="button" data-remove-filter="search" title="Clear search">&times;</button>
          </span>
        `);
      }
      if (activeFilters.sort !== "featured") {
        const sortNames = {
          price_asc: "Price: Low to High",
          price_desc: "Price: High to Low",
          name_asc: "Title: A to Z",
          name_desc: "Title: Z to A",
        };
        tags.push(`
          <span class="filter-tag">
            Sort: ${sortNames[activeFilters.sort] || activeFilters.sort}
            <button type="button" data-remove-filter="sort" title="Reset sort">&times;</button>
          </span>
        `);
      }

      if (tags.length > 0) {
        activeFilterTags.hidden = false;
        activeFilterTags.innerHTML = tags.join("");
        activeFilterTags.querySelectorAll("[data-remove-filter]").forEach((btn) => {
          btn.addEventListener("click", () => {
            const f = btn.getAttribute("data-remove-filter");
            if (f === "category") {
              activeFilters.category = "all";
              if (filterCategory) filterCategory.value = "all";
            } else if (f === "instock") {
              activeFilters.inStock = false;
              if (filterInStock) filterInStock.checked = false;
            } else if (f === "search") {
              activeFilters.search = "";
              if (searchInput) searchInput.value = "";
            } else if (f === "sort") {
              activeFilters.sort = "featured";
              if (filterSort) filterSort.value = "featured";
            }
            fetchProducts();
          });
        });
      } else {
        activeFilterTags.hidden = true;
      }
    }
  }

  function openProductDetail(p) {
    if (!productDetailModal || !p) return;
    currentDetailProduct = p;

    if (detailProductType) detailProductType.textContent = (p.type === "book" ? "Book Details" : "Stationery Details");
    if (detailProductTitle) detailProductTitle.textContent = p.title;
    if (detailCoverTitle) detailCoverTitle.textContent = p.title;
    if (detailCoverBox) {
      detailCoverBox.className = `product-cover detail-cover ${p.cover_class || (p.type === 'stationery' ? 'cover-stat-1' : 'cover-book-1')}`;
    }
    if (detailSku) detailSku.textContent = `SKU: ${p.sku}`;
    if (detailPrice) detailPrice.textContent = money(p.price);

    if (detailDiscount) {
      if (p.discount_price && p.discount_price > 0 && p.discount_price < p.price) {
        detailDiscount.hidden = false;
        detailDiscount.textContent = `Special student offer: Save ${money(p.price - p.discount_price)}`;
      } else {
        detailDiscount.hidden = true;
      }
    }

    if (detailStockBadge) {
      if (p.stock_quantity > 0) {
        detailStockBadge.className = "stock-badge stock-ok";
        detailStockBadge.textContent = `${p.stock_quantity} units available`;
      } else {
        detailStockBadge.className = "stock-badge stock-out";
        detailStockBadge.textContent = "Currently Out of Stock";
      }
    }

    if (detailDescription) detailDescription.textContent = p.description || "High-quality academic book or stationery item.";

    if (detailMetaList) {
      const items = [];
      if (p.type === "book") {
        if (p.author) items.push(`<dt>Author</dt><dd>${escapeHtml(p.author)}</dd>`);
        if (p.category) items.push(`<dt>Category</dt><dd>${escapeHtml(p.category)}</dd>`);
        if (p.isbn) items.push(`<dt>ISBN</dt><dd><code>${escapeHtml(p.isbn)}</code></dd>`);
        if (p.publisher) items.push(`<dt>Publisher</dt><dd>${escapeHtml(p.publisher)}</dd>`);
        if (p.edition) items.push(`<dt>Edition</dt><dd>${escapeHtml(p.edition)}</dd>`);
        if (p.language) items.push(`<dt>Language</dt><dd>${escapeHtml(p.language)}</dd>`);
      } else {
        if (p.brand) items.push(`<dt>Brand</dt><dd>${escapeHtml(p.brand)}</dd>`);
        if (p.category) items.push(`<dt>Category</dt><dd>${escapeHtml(p.category)}</dd>`);
        if (p.color) items.push(`<dt>Color</dt><dd>${escapeHtml(p.color)}</dd>`);
        if (p.size) items.push(`<dt>Size / Format</dt><dd>${escapeHtml(p.size)}</dd>`);
        if (p.specifications) items.push(`<dt>Specs</dt><dd>${escapeHtml(p.specifications)}</dd>`);
      }
      detailMetaList.innerHTML = items.join("");
    }

    if (detailQtyInput) detailQtyInput.value = "1";
    if (detailAddCartBtn) {
      detailAddCartBtn.disabled = (p.stock_quantity <= 0);
      detailAddCartBtn.textContent = (p.stock_quantity > 0 ? "Add to Cart" : "Out of Stock");
    }

    productDetailModal.showModal();
  }

  function cardMarkup(product) {
    const stockClass = product.stock_quantity > 0 ? "stock-ok" : "stock-out";
    const stockLabel = product.stock_quantity > 0 ? `${product.stock_quantity} in stock` : "Out of stock";

    return `
      <article class="product-card" data-id="${product.id}" data-type="${product.type}">
        <div class="product-cover ${product.cover_class || 'cover-book-1'}" aria-hidden="true">
          <span>${escapeHtml(product.title)}</span>
        </div>
        <div class="product-body">
          <p class="product-meta">${escapeHtml(product.meta || product.category || '')}</p>
          <h3>${escapeHtml(product.title)}</h3>
          <p class="product-desc">${escapeHtml(product.description || '')}</p>
          <div class="stock-badge ${stockClass}">${stockLabel}</div>
          <div class="price-row">
            <p class="price">${money(product.price)}</p>
            <div style="display:flex;gap:6px;align-items:center;">
              <button class="btn btn-xs btn-outline" type="button" data-detail="${product.id}" title="View full specifications">Details</button>
              <button class="add-cart" type="button" data-add="${product.id}" ${product.stock_quantity <= 0 ? 'disabled' : ''}>
                ${product.stock_quantity > 0 ? 'Add' : 'Out'}
              </button>
            </div>
          </div>
        </div>
      </article>
    `;
  }

  function escapeHtml(str) {
    if (!str) return "";
    return String(str)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }

  function renderCatalog() {
    if (!booksGrid || !stationeryGrid) return;

    const books = products.filter((item) => item.type === "book");
    const stats = products.filter((item) => item.type === "stationery");

    booksGrid.innerHTML = books.length
      ? books.map(cardMarkup).join("")
      : '<p class="empty-state">No books match your selected filters.</p>';

    stationeryGrid.innerHTML = stats.length
      ? stats.map(cardMarkup).join("")
      : '<p class="empty-state">No stationery items match your selected filters.</p>';
  }

  function cartQuantity(cart) {
    return cart.reduce((sum, line) => sum + line.qty, 0);
  }

  /* Server-verified Cart Rendering */
  async function renderCart() {
    const cart = readCart();
    if (cartCount) cartCount.textContent = String(cartQuantity(cart));

    if (!cart.length) {
      if (cartItems) cartItems.innerHTML = '<li class="cart-empty-line">Your cart is empty.</li>';
      if (cartSubtotal) cartSubtotal.textContent = money(0);
      if (cartDelivery) cartDelivery.textContent = money(0);
      if (cartTotal) cartTotal.textContent = money(0);
      if (checkoutOpenBtn) checkoutOpenBtn.disabled = true;
      return;
    }

    if (checkoutOpenBtn) checkoutOpenBtn.disabled = false;

    /* Validate with C backend */
    let serverTotal = 0;
    let serverSubtotal = 0;
    let serverDelivery = 60;
    let verifiedMap = {};

    try {
      const resp = await fetch("/api/cart/validate", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(cart),
      });
      if (resp.ok) {
        const data = await resp.json();
        serverTotal = data.total;
        serverSubtotal = data.subtotal;
        serverDelivery = data.delivery_charge;
        if (data.items) {
          data.items.forEach((item) => {
            verifiedMap[item.id] = item;
          });
        }
      }
    } catch (err) {
      /* Fallback calculation */
      serverSubtotal = cart.reduce((sum, line) => {
        const p = products.find((x) => x.id === line.id || x.sku === line.id);
        return sum + (p ? p.price * line.qty : 0);
      }, 0);
      serverTotal = serverSubtotal + (serverSubtotal > 0 ? 60 : 0);
    }

    if (cartSubtotal) cartSubtotal.textContent = money(serverSubtotal);
    if (cartDelivery) cartDelivery.textContent = money(serverDelivery);
    if (cartTotal) cartTotal.textContent = money(serverTotal);

    if (cartItems) {
      cartItems.innerHTML = cart
        .map((line) => {
          const product = products.find((item) => item.id === line.id || item.sku === line.id);
          if (!product) return "";
          const v = verifiedMap[line.id];
          const warnHtml = v && v.stock_warning
            ? `<div class="cart-stock-warning">⚠️ Only ${v.available_stock} available</div>`
            : "";

          return `
            <li class="cart-line">
              <div class="cart-line-info">
                <strong>${escapeHtml(product.title)}</strong>
                <div class="cart-qty-row">
                  <button type="button" class="btn-qty" data-dec="${line.id}" aria-label="Decrease quantity">−</button>
                  <span class="qty-num">${line.qty}</span>
                  <button type="button" class="btn-qty" data-inc="${line.id}" aria-label="Increase quantity">+</button>
                  <span class="unit-price">× ${money(product.price)}</span>
                </div>
                ${warnHtml}
              </div>
              <button type="button" class="btn-remove" data-remove="${line.id}">Remove</button>
            </li>
          `;
        })
        .join("");
    }
  }

  function addToCart(id, requestedQty = 1) {
    const numId = Number(id) || id;
    const cart = readCart();
    const existing = cart.find((line) => line.id === numId);

    const product = products.find((item) => item.id === numId || item.sku === id);
    if (!product || product.stock_quantity <= 0) {
      showToast("Sorry, this item is out of stock!");
      return false;
    }

    const currentQtyInCart = existing ? existing.qty : 0;
    if (currentQtyInCart + requestedQty > product.stock_quantity) {
      const allowedAdd = Math.max(0, product.stock_quantity - currentQtyInCart);
      if (allowedAdd <= 0) {
        showToast(`Stock limit reached! Max available: ${product.stock_quantity}`);
        return false;
      }
      if (existing) {
        existing.qty = product.stock_quantity;
      } else {
        cart.push({ id: numId, qty: allowedAdd });
      }
      showToast(`Added ${allowedAdd} item(s). Max stock reached (${product.stock_quantity})`);
    } else {
      if (existing) {
        existing.qty += requestedQty;
      } else {
        cart.push({ id: numId, qty: requestedQty });
      }
      showToast((product ? product.title : "Item") + " added to cart");
    }

    writeCart(cart);
    renderCart();
    return true;
  }

  function changeCartQty(id, delta) {
    const numId = Number(id) || id;
    let cart = readCart();
    const item = cart.find((x) => x.id === numId);
    if (!item) return;

    const product = products.find((x) => x.id === numId || x.sku === id);
    if (delta > 0 && product && item.qty + delta > product.stock_quantity) {
      showToast(`Maximum available stock is ${product.stock_quantity} units.`);
      return;
    }

    item.qty += delta;
    if (item.qty <= 0) {
      cart = cart.filter((x) => x.id !== numId);
    }
    writeCart(cart);
    renderCart();
  }

  /* Orders History from SQLite with Visual Tracking Progress Bar */
  async function loadOrders() {
    if (!ordersList) return;
    try {
      const resp = await fetch("/api/orders");
      if (resp.ok) {
        const orders = await resp.json();
        if (Array.isArray(orders) && orders.length > 0) {
          const stages = ["Pending", "Confirmed", "Processing", "Shipped", "Delivered"];

          ordersList.innerHTML = orders
            .map((order) => {
              const statusClass = `status-${(order.status || 'pending').toLowerCase()}`;
              const itemsList = (order.items || [])
                .map((i) => `<li>${escapeHtml(i.title)} × ${i.quantity} (${money(i.line_total)})</li>`)
                .join("");

              const currentIdx = stages.indexOf(order.status);
              const isCancelled = order.status === "Cancelled";

              let trackerHtml = "";
              if (!isCancelled) {
                const progressPercent = currentIdx >= 0 ? (currentIdx / (stages.length - 1)) * 100 : 0;
                const stepsHtml = stages
                  .map((stage, idx) => {
                    let stepClass = "";
                    if (idx < currentIdx) stepClass = "completed";
                    else if (idx === currentIdx) stepClass = "active";
                    return `
                      <div class="tracker-step ${stepClass}">
                        <div class="step-dot">${idx < currentIdx ? '✓' : idx + 1}</div>
                        <span class="step-name">${stage}</span>
                      </div>
                    `;
                  })
                  .join("");

                trackerHtml = `
                  <div class="order-tracker">
                    <div class="tracker-track"><div class="tracker-progress" style="width: ${progressPercent}%;"></div></div>
                    ${stepsHtml}
                  </div>
                `;
              } else {
                trackerHtml = `
                  <div style="background:rgba(153,27,27,0.08);color:#991b1b;padding:8px 14px;border-radius:8px;font-size:0.85rem;font-weight:600;margin:12px 0;">
                    Order Cancelled
                  </div>
                `;
              }

              return `
                <article class="order-card">
                  <div class="order-card-header">
                    <div>
                      <span class="order-num">${escapeHtml(order.order_number)}</span>
                      <span class="order-date">${escapeHtml(order.created_at || '')}</span>
                    </div>
                    <span class="order-status-badge ${statusClass}">${escapeHtml(order.status)}</span>
                  </div>
                  ${trackerHtml}
                  <div class="order-details-grid">
                    <div>
                      <p class="order-label">Recipient:</p>
                      <p class="order-val"><strong>${escapeHtml(order.customer_name)}</strong> (${escapeHtml(order.phone)})</p>
                      <p class="order-label">Address:</p>
                      <p class="order-val">${escapeHtml(order.delivery_address)}</p>
                    </div>
                    <div>
                      <p class="order-label">Items:</p>
                      <ul class="order-item-list">${itemsList || '<li>Standard order items</li>'}</ul>
                      <p class="order-label">Payment: <span>${escapeHtml(order.payment_method)} (${escapeHtml(order.payment_status)})</span></p>
                      <p class="order-total-line">Total: <strong>${money(order.total_amount)}</strong></p>
                    </div>
                  </div>
                </article>
              `;
            })
            .join("");
          return;
        }
      }
    } catch (err) {
      console.warn("Could not load orders:", err);
    }

    ordersList.innerHTML = `
      <div class="orders-empty-box">
        <p>No orders placed yet.</p>
        <p><small>Add books or stationery to your cart and checkout to create real order records.</small></p>
      </div>
    `;
  }

  /* Authentication Status */
  async function checkAuth() {
    try {
      const resp = await fetch("/api/auth/me");
      if (resp.ok) {
        const data = await resp.json();
        if (data.authenticated) {
          currentUser = data;
          if (authToggle) {
            authToggle.textContent = `${data.name.split(' ')[0]} (${data.role})`;
            authToggle.classList.add("auth-active");
          }
          const nameInput = document.getElementById("cust-name");
          if (nameInput && !nameInput.value) nameInput.value = data.name;
          return;
        }
      }
    } catch (err) {
      console.warn("Auth check error:", err);
    }
    currentUser = null;
    if (authToggle) {
      authToggle.textContent = "Sign In";
      authToggle.classList.remove("auth-active");
    }
  }

  async function demoLogin(role) {
    try {
      const resp = await fetch("/api/auth/demo-login", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ role: role }),
      });
      if (resp.ok) {
        const data = await resp.json();
        showToast(data.message || "Logged in successfully!");
        if (authModal) authModal.close();
        await checkAuth();
        await loadOrders();
      }
    } catch (err) {
      showToast("Login failed: " + err.message);
    }
  }

  /* Global handler for Google Sign-In */
  window.handleGoogleSignIn = async function (response) {
    try {
      const resp = await fetch("/api/auth/google", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ credential: response.credential }),
      });
      if (resp.ok) {
        const data = await resp.json();
        showToast("Signed in as " + data.name);
        if (authModal) authModal.close();
        await checkAuth();
        await loadOrders();
      } else {
        showToast("Google authentication failed.");
      }
    } catch (err) {
      showToast("Authentication error: " + err.message);
    }
  };

  /* Event Listeners */
  document.addEventListener("click", (event) => {
    const add = event.target.closest("[data-add]");
    if (add) {
      addToCart(add.getAttribute("data-add"));
      return;
    }

    const detailBtn = event.target.closest("[data-detail]");
    if (detailBtn) {
      const prodId = Number(detailBtn.getAttribute("data-detail")) || detailBtn.getAttribute("data-detail");
      const prod = products.find((p) => p.id === prodId || p.sku === prodId);
      if (prod) openProductDetail(prod);
      return;
    }

    const card = event.target.closest(".product-card");
    if (card && !event.target.closest("button")) {
      const prodId = Number(card.getAttribute("data-id")) || card.getAttribute("data-id");
      const prod = products.find((p) => p.id === prodId || p.sku === prodId);
      if (prod) openProductDetail(prod);
      return;
    }

    const remove = event.target.closest("[data-remove]");
    if (remove) {
      const id = remove.getAttribute("data-remove");
      writeCart(readCart().filter((line) => line.id != id && line.id !== Number(id)));
      renderCart();
      return;
    }

    const inc = event.target.closest("[data-inc]");
    if (inc) {
      changeCartQty(inc.getAttribute("data-inc"), 1);
      return;
    }

    const dec = event.target.closest("[data-dec]");
    if (dec) {
      changeCartQty(dec.getAttribute("data-dec"), -1);
      return;
    }
  });

  /* Detail Modal Listeners */
  if (detailQtyDec) {
    detailQtyDec.addEventListener("click", () => {
      if (!detailQtyInput) return;
      let val = parseInt(detailQtyInput.value, 10) || 1;
      if (val > 1) {
        detailQtyInput.value = String(val - 1);
      }
    });
  }

  if (detailQtyInc) {
    detailQtyInc.addEventListener("click", () => {
      if (!detailQtyInput || !currentDetailProduct) return;
      let val = parseInt(detailQtyInput.value, 10) || 1;
      if (val < currentDetailProduct.stock_quantity) {
        detailQtyInput.value = String(val + 1);
      } else {
        showToast(`Maximum available stock is ${currentDetailProduct.stock_quantity}`);
      }
    });
  }

  if (detailAddCartBtn) {
    detailAddCartBtn.addEventListener("click", () => {
      if (!currentDetailProduct) return;
      const qty = parseInt(detailQtyInput.value, 10) || 1;
      addToCart(currentDetailProduct.id, qty);
      if (productDetailModal) productDetailModal.close();
    });
  }

  if (detailModalCloseBtn) {
    detailModalCloseBtn.addEventListener("click", () => {
      if (productDetailModal) productDetailModal.close();
    });
  }

  if (productDetailModal) {
    productDetailModal.addEventListener("click", (e) => {
      const rect = productDetailModal.getBoundingClientRect();
      const isInDialog = (rect.top <= e.clientY && e.clientY <= rect.top + rect.height &&
        rect.left <= e.clientX && e.clientX <= rect.left + rect.width);
      if (!isInDialog) {
        productDetailModal.close();
      }
    });
  }

  /* Catalog Filter & Sort Listeners */
  if (filterCategory) {
    filterCategory.addEventListener("change", () => {
      activeFilters.category = filterCategory.value;
      fetchProducts();
    });
  }

  if (filterSort) {
    filterSort.addEventListener("change", () => {
      activeFilters.sort = filterSort.value;
      fetchProducts();
    });
  }

  if (filterInStock) {
    filterInStock.addEventListener("change", () => {
      activeFilters.inStock = filterInStock.checked;
      fetchProducts();
    });
  }

  if (btnResetFilters) {
    btnResetFilters.addEventListener("click", () => {
      activeFilters = {
        category: "all",
        sort: "featured",
        inStock: false,
        search: "",
      };
      if (filterCategory) filterCategory.value = "all";
      if (filterSort) filterSort.value = "featured";
      if (filterInStock) filterInStock.checked = false;
      if (searchInput) searchInput.value = "";
      fetchProducts();
      showToast("Filters reset");
    });
  }

  if (searchForm && searchInput) {
    searchForm.addEventListener("submit", (event) => {
      event.preventDefault();
      activeFilters.search = searchInput.value.trim();
      fetchProducts();
      const booksEl = document.getElementById("books");
      if (booksEl) booksEl.scrollIntoView({ behavior: "smooth" });
    });

    searchInput.addEventListener("input", () => {
      activeFilters.search = searchInput.value.trim();
      fetchProducts();
    });
  }

  if (cartOpen && cartDrawer) {
    cartOpen.addEventListener("click", () => {
      cartDrawer.hidden = false;
      renderCart();
    });
  }

  if (cartClose && cartDrawer) {
    cartClose.addEventListener("click", () => {
      cartDrawer.hidden = true;
    });
  }

  if (cartDrawer) {
    cartDrawer.addEventListener("click", (event) => {
      if (event.target === cartDrawer) cartDrawer.hidden = true;
    });
  }

  if (menuToggle && primaryNav) {
    menuToggle.addEventListener("click", () => {
      const open = primaryNav.classList.toggle("is-open");
      menuToggle.setAttribute("aria-expanded", String(open));
    });

    primaryNav.querySelectorAll("a").forEach((link) => {
      link.addEventListener("click", () => {
        primaryNav.classList.remove("is-open");
        menuToggle.setAttribute("aria-expanded", "false");
      });
    });
  }

  /* Checkout Dialog */
  if (checkoutOpenBtn && checkoutModal) {
    checkoutOpenBtn.addEventListener("click", () => {
      if (cartDrawer) cartDrawer.hidden = true;
      if (cartTotal && checkoutTotalVal) {
        checkoutTotalVal.textContent = cartTotal.textContent;
      }
      checkoutModal.showModal();
    });
  }

  if (checkoutCloseBtn && checkoutModal) {
    checkoutCloseBtn.addEventListener("click", () => checkoutModal.close());
  }

  if (checkoutCancelBtn && checkoutModal) {
    checkoutCancelBtn.addEventListener("click", () => checkoutModal.close());
  }

  if (checkoutForm) {
    checkoutForm.addEventListener("submit", async (e) => {
      e.preventDefault();
      const cart = readCart();
      if (!cart.length) {
        showToast("Your cart is empty!");
        return;
      }

      const formData = new FormData(checkoutForm);
      const payload = {
        customer_name: formData.get("customer_name"),
        phone: formData.get("phone"),
        delivery_address: formData.get("delivery_address"),
        payment_method: formData.get("payment_method"),
        items: cart,
      };

      const submitBtn = document.getElementById("place-order-submit-btn");
      if (submitBtn) {
        submitBtn.disabled = true;
        submitBtn.textContent = "Processing Order…";
      }

      try {
        const resp = await fetch("/api/orders", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(payload),
        });

        const data = await resp.json();
        if (resp.ok && data.status === "success") {
          writeCart([]);
          renderCart();
          if (checkoutModal) checkoutModal.close();
          checkoutForm.reset();
          showToast(`Order Placed! Order #${data.order_number}`);
          await loadOrders();
          await fetchProducts(); // Refresh stock counts in UI

          const ordersEl = document.getElementById("orders");
          if (ordersEl) ordersEl.scrollIntoView({ behavior: "smooth" });
        } else {
          showToast("Checkout failed: " + (data.message || "Unknown error"));
        }
      } catch (err) {
        showToast("Network error: " + err.message);
      } finally {
        if (submitBtn) {
          submitBtn.disabled = false;
          submitBtn.textContent = "Confirm Order";
        }
      }
    });
  }

  /* Auth Dialog */
  if (authToggle && authModal) {
    authToggle.addEventListener("click", () => {
      if (currentUser) {
        /* If already logged in, prompt logout */
        if (confirm(`Logged in as ${currentUser.name}. Do you want to log out?`)) {
          fetch("/api/auth/logout", { method: "POST" }).then(() => {
            currentUser = null;
            checkAuth();
            showToast("Logged out successfully");
            loadOrders();
          });
        }
      } else {
        authModal.showModal();
      }
    });
  }

  if (authCloseBtn && authModal) {
    authCloseBtn.addEventListener("click", () => authModal.close());
  }

  if (demoStudentBtn) {
    demoStudentBtn.addEventListener("click", () => demoLogin("customer"));
  }

  if (demoAdminBtn) {
    demoAdminBtn.addEventListener("click", () => demoLogin("admin"));
  }

  if (refreshOrdersBtn) {
    refreshOrdersBtn.addEventListener("click", () => {
      loadOrders();
      showToast("Order list updated");
    });
  }

  /* Initialization */
  loadCategories();
  fetchProducts();
  renderCart();
  checkAuth();
  loadOrders();
})();
