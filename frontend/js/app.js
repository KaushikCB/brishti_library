(function () {
  const products = [
    {
      id: "b1",
      type: "book",
      title: "Monsoon Algorithms",
      meta: "N. Rahman · CS",
      price: 890,
      cover: "cover-book-1",
      blurb: "Sorting storms, graphs, and exam week.",
    },
    {
      id: "b2",
      type: "book",
      title: "The Quiet Stack",
      meta: "A. Chowdhury · Fiction",
      price: 650,
      cover: "cover-book-2",
      blurb: "A campus novel about libraries after hours.",
    },
    {
      id: "b3",
      type: "book",
      title: "Paper Weather",
      meta: "L. Ahmed · Essays",
      price: 720,
      cover: "cover-book-3",
      blurb: "Short essays on reading in the rain.",
    },
    {
      id: "b4",
      type: "book",
      title: "Graph of Home",
      meta: "S. Islam · CS",
      price: 980,
      cover: "cover-book-4",
      blurb: "Paths, trees, and finding your way back.",
    },
    {
      id: "s1",
      type: "stationery",
      title: "Rain Hall Notebook",
      meta: "A5 · 120 pages",
      price: 220,
      cover: "cover-stat-1",
      blurb: "Dot grid paper for lectures and labs.",
    },
    {
      id: "s2",
      type: "stationery",
      title: "Forest Ink Pen",
      meta: "Fine nib",
      price: 340,
      cover: "cover-stat-2",
      blurb: "Smooth gel ink for long problem sets.",
    },
    {
      id: "s3",
      type: "stationery",
      title: "Goldclip Highlighters",
      meta: "Set of 4",
      price: 180,
      cover: "cover-stat-3",
      blurb: "Soft tones that do not bleed through.",
    },
    {
      id: "s4",
      type: "stationery",
      title: "Desk Storm Sticky Notes",
      meta: "3 packs",
      price: 150,
      cover: "cover-stat-4",
      blurb: "Reminders for due dates and page marks.",
    },
  ];

  const money = (n) => "৳" + n.toLocaleString("en-BD");
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
  const cartTotal = document.getElementById("cart-total");
  const menuToggle = document.getElementById("menu-toggle");
  const primaryNav = document.getElementById("primary-nav");
  const toast = document.getElementById("toast");

  const CART_KEY = "brishtilibrary-cart";

  function readCart() {
    try {
      return JSON.parse(localStorage.getItem(CART_KEY)) || [];
    } catch (err) {
      return [];
    }
  }

  function writeCart(cart) {
    localStorage.setItem(CART_KEY, JSON.stringify(cart));
  }

  function cardMarkup(product) {
    return `
      <article class="product-card" data-id="${product.id}" data-type="${product.type}">
        <div class="product-cover ${product.cover}" aria-hidden="true">
          <span>${product.title}</span>
        </div>
        <div class="product-body">
          <p class="product-meta">${product.meta}</p>
          <h3>${product.title}</h3>
          <p>${product.blurb}</p>
          <div class="price-row">
            <p class="price">${money(product.price)}</p>
            <button class="add-cart" type="button" data-add="${product.id}">Add to Cart</button>
          </div>
        </div>
      </article>
    `;
  }

  function renderCatalog() {
    booksGrid.innerHTML = products
      .filter((item) => item.type === "book")
      .map(cardMarkup)
      .join("");
    stationeryGrid.innerHTML = products
      .filter((item) => item.type === "stationery")
      .map(cardMarkup)
      .join("");
  }

  function cartQuantity(cart) {
    return cart.reduce((sum, line) => sum + line.qty, 0);
  }

  function renderCart() {
    const cart = readCart();
    cartCount.textContent = String(cartQuantity(cart));

    if (!cart.length) {
      cartItems.innerHTML = "<li>Your cart is empty.</li>";
      cartTotal.textContent = money(0);
      return;
    }

    cartItems.innerHTML = cart
      .map((line) => {
        const product = products.find((item) => item.id === line.id);
        if (!product) return "";
        return `
          <li class="cart-line">
            <div>
              <strong>${product.title}</strong>
              <div>${line.qty} × ${money(product.price)}</div>
            </div>
            <button type="button" data-remove="${product.id}">Remove</button>
          </li>
        `;
      })
      .join("");

    const total = cart.reduce((sum, line) => {
      const product = products.find((item) => item.id === line.id);
      return sum + (product ? product.price * line.qty : 0);
    }, 0);
    cartTotal.textContent = money(total);
  }

  function showToast(message) {
    toast.hidden = false;
    toast.textContent = message;
    window.clearTimeout(showToast.timer);
    showToast.timer = window.setTimeout(() => {
      toast.hidden = true;
    }, 1800);
  }

  function addToCart(id) {
    const cart = readCart();
    const existing = cart.find((line) => line.id === id);
    if (existing) {
      existing.qty += 1;
    } else {
      cart.push({ id: id, qty: 1 });
    }
    writeCart(cart);
    renderCart();
    const product = products.find((item) => item.id === id);
    showToast((product ? product.title : "Item") + " added to cart");
  }

  function filterProducts(query) {
    const q = query.trim().toLowerCase();
    const cards = document.querySelectorAll(".product-card");
    let visible = 0;

    cards.forEach((card) => {
      const text = card.textContent.toLowerCase();
      const match = !q || text.includes(q);
      card.classList.toggle("is-hidden", !match);
      if (match) visible += 1;
    });

    searchStatus.hidden = !q;
    searchStatus.textContent = q
      ? visible
        ? visible + " result(s) for “" + query.trim() + "”"
        : "No products match “" + query.trim() + "”"
      : "";
  }

  document.addEventListener("click", (event) => {
    const add = event.target.closest("[data-add]");
    if (add) addToCart(add.getAttribute("data-add"));

    const remove = event.target.closest("[data-remove]");
    if (remove) {
      writeCart(readCart().filter((line) => line.id !== remove.getAttribute("data-remove")));
      renderCart();
    }
  });

  searchForm.addEventListener("submit", (event) => {
    event.preventDefault();
    filterProducts(searchInput.value);
    document.getElementById("books").scrollIntoView({ behavior: "smooth" });
  });

  searchInput.addEventListener("input", () => {
    filterProducts(searchInput.value);
  });

  cartOpen.addEventListener("click", () => {
    cartDrawer.hidden = false;
  });

  cartClose.addEventListener("click", () => {
    cartDrawer.hidden = true;
  });

  cartDrawer.addEventListener("click", (event) => {
    if (event.target === cartDrawer) cartDrawer.hidden = true;
  });

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

  renderCatalog();
  renderCart();
})();
