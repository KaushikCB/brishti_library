(function () {
  /* Tab Navigation */
  const pills = document.querySelectorAll(".lab-nav-pills .pill");
  const tabPanels = document.querySelectorAll(".lab-tab-panel");

  pills.forEach((pill) => {
    pill.addEventListener("click", () => {
      pills.forEach((p) => p.classList.remove("active"));
      tabPanels.forEach((t) => t.classList.remove("active"));
      pill.classList.add("active");
      const targetId = pill.getAttribute("data-tab");
      const targetPanel = document.getElementById(targetId);
      if (targetPanel) targetPanel.classList.add("active");
    });
  });

  function parseNumberArray(str) {
    return str
      .split(/[, ]+/)
      .map((s) => s.trim())
      .filter((s) => s.length > 0 && !isNaN(Number(s)))
      .map((s) => Number(s));
  }

  /* ----------------------------------------------------
     TAB 1: SEARCHING ALGORITHMS
  ---------------------------------------------------- */
  const btnRunLinear = document.getElementById("btn-run-linear");
  const linearArrInput = document.getElementById("linear-arr-input");
  const linearTargetInput = document.getElementById("linear-target-input");
  const linearOutputBox = document.getElementById("linear-output-box");
  const linearResIdx = document.getElementById("linear-res-idx");
  const linearResCmp = document.getElementById("linear-res-cmp");
  const linearResTime = document.getElementById("linear-res-time");
  const linearChips = document.getElementById("linear-chips");

  if (btnRunLinear) {
    btnRunLinear.addEventListener("click", async () => {
      const arr = parseNumberArray(linearArrInput.value);
      const target = Number(linearTargetInput.value);

      if (!arr.length) {
        alert("Please enter a valid array of numbers.");
        return;
      }

      btnRunLinear.disabled = true;
      btnRunLinear.textContent = "Running in C…";

      try {
        const resp = await fetch("/api/algorithms/search/linear", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ array: arr, target: target }),
        });
        const data = await resp.json();

        linearOutputBox.hidden = false;
        linearResIdx.textContent = data.found_index >= 0 ? data.found_index : "Not Found (-1)";
        linearResCmp.textContent = data.comparisons;
        linearResTime.textContent = data.time_microseconds.toFixed(2);

        /* Render chips */
        linearChips.innerHTML = arr
          .map((val, idx) => {
            const step = data.steps.find((s) => s.index === idx);
            let cls = "chip";
            if (step) {
              cls += step.matched ? " chip-matched" : " chip-scanned";
            }
            return `
              <div class="${cls}">
                <span class="chip-idx">[${idx}]</span>
                <span class="chip-val">${val}</span>
              </div>
            `;
          })
          .join("");
      } catch (err) {
        alert("Execution error: " + err.message);
      } finally {
        btnRunLinear.disabled = false;
        btnRunLinear.textContent = "Execute C Linear Search";
      }
    });
  }

  const btnRunBinary = document.getElementById("btn-run-binary");
  const binaryArrInput = document.getElementById("binary-arr-input");
  const binaryTargetInput = document.getElementById("binary-target-input");
  const binaryOutputBox = document.getElementById("binary-output-box");
  const binaryResIdx = document.getElementById("binary-res-idx");
  const binaryResCmp = document.getElementById("binary-res-cmp");
  const binaryResIter = document.getElementById("binary-res-iter");
  const binaryResTime = document.getElementById("binary-res-time");
  const binaryStepsList = document.getElementById("binary-steps-list");

  if (btnRunBinary) {
    btnRunBinary.addEventListener("click", async () => {
      const arr = parseNumberArray(binaryArrInput.value);
      const target = Number(binaryTargetInput.value);

      if (!arr.length) {
        alert("Please enter a valid sorted array of numbers.");
        return;
      }

      /* Auto-sort if user entered unsorted numbers */
      arr.sort((a, b) => a - b);
      binaryArrInput.value = arr.join(", ");

      btnRunBinary.disabled = true;
      btnRunBinary.textContent = "Running in C…";

      try {
        const resp = await fetch("/api/algorithms/search/binary", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ array: arr, target: target }),
        });
        const data = await resp.json();

        binaryOutputBox.hidden = false;
        binaryResIdx.textContent = data.found_index >= 0 ? data.found_index : "Not Found (-1)";
        binaryResCmp.textContent = data.comparisons;
        binaryResIter.textContent = data.iterations;
        binaryResTime.textContent = data.time_microseconds.toFixed(2);

        binaryStepsList.innerHTML = (data.steps || [])
          .map((st, i) => {
            const isMatch = st.comparison_result === 0;
            const cmpSymbol = isMatch ? "=" : (st.comparison_result < 0 ? "<" : ">");
            return `
              <div class="binary-step-item ${isMatch ? 'is-match' : ''}">
                <span>Step ${i + 1}: Low=${st.low}, High=${st.high}</span>
                <span>Mid [${st.mid}] = <strong>${st.mid_value}</strong></span>
                <span>Target ${target} ${cmpSymbol} ${st.mid_value}</span>
                <span>${isMatch ? '✓ Found First Match!' : (st.comparison_result < 0 ? 'Search Left Half' : 'Search Right Half')}</span>
              </div>
            `;
          })
          .join("");
      } catch (err) {
        alert("Execution error: " + err.message);
      } finally {
        btnRunBinary.disabled = false;
        btnRunBinary.textContent = "Execute C Binary Search";
      }
    });
  }

  /* Preset buttons */
  document.querySelectorAll("[data-preset]").forEach((btn) => {
    btn.addEventListener("click", () => {
      const preset = btn.getAttribute("data-preset");
      const type = btn.getAttribute("data-type");

      if (preset === "linear" && type === "random") {
        const rnd = Array.from({ length: 10 }, () => Math.floor(Math.random() * 90) + 10);
        linearArrInput.value = rnd.join(", ");
        linearTargetInput.value = rnd[Math.floor(Math.random() * rnd.length)];
      } else if (preset === "binary" && type === "sort-current") {
        const arr = parseNumberArray(binaryArrInput.value);
        arr.sort((a, b) => a - b);
        binaryArrInput.value = arr.join(", ");
      }
    });
  });

  /* ----------------------------------------------------
     TAB 2: SORTING ALGORITHMS
  ---------------------------------------------------- */
  const btnRunSort = document.getElementById("btn-run-sort");
  const sortArrInput = document.getElementById("sort-arr-input");
  const sortAlgoSelect = document.getElementById("sort-algo-select");
  const sortResultsArea = document.getElementById("sort-results-area");

  const sortResultTitle = document.getElementById("sort-result-title");
  const sortComplexityNote = document.getElementById("sort-complexity-note");
  const sortMetricCmp = document.getElementById("sort-metric-cmp");
  const sortMetricSwaps = document.getElementById("sort-metric-swaps");
  const sortMetricShifts = document.getElementById("sort-metric-shifts");
  const sortMetricTime = document.getElementById("sort-metric-time");

  const btnPrevPass = document.getElementById("btn-prev-pass");
  const btnNextPass = document.getElementById("btn-next-pass");
  const btnPlayPasses = document.getElementById("btn-play-passes");
  const currPassNum = document.getElementById("curr-pass-num");
  const totalPassNum = document.getElementById("total-pass-num");
  const arrayBarsContainer = document.getElementById("array-bars-container");
  const passDetailBox = document.getElementById("pass-detail-box");

  let currentSortData = null;
  let currentPassIdx = 0;
  let playInterval = null;

  function renderPass(passIdx) {
    if (!currentSortData || !currentSortData.passes || !currentSortData.passes.length) return;

    currentPassIdx = Math.max(0, Math.min(passIdx, currentSortData.passes.length - 1));
    const pass = currentSortData.passes[currentPassIdx];
    currPassNum.textContent = String(currentPassIdx + 1);
    totalPassNum.textContent = String(currentSortData.passes.length);

    const maxVal = Math.max(...pass.state, 1);
    const isLastPass = currentPassIdx === currentSortData.passes.length - 1;

    arrayBarsContainer.innerHTML = pass.state
      .map((val, idx) => {
        const heightPct = Math.max(12, Math.round((val / maxVal) * 160));
        let barClass = "bar";
        if (isLastPass) {
          barClass += " bar-sorted";
        } else if (idx === pass.active_1) {
          barClass += " bar-active";
        } else if (idx === pass.active_2) {
          barClass += " bar-pivot";
        }

        return `
          <div class="bar-col">
            <span class="bar-val">${val}</span>
            <div class="${barClass}" style="height: ${heightPct}px;"></div>
            <span class="bar-idx">[${idx}]</span>
          </div>
        `;
      })
      .join("");

    passDetailBox.innerHTML = `
      <strong>Pass ${pass.pass_number}:</strong> 
      Comparisons in this pass: <strong>${pass.comparisons}</strong> | 
      Swaps: <strong>${pass.swaps}</strong> | 
      Shifts: <strong>${pass.shifts}</strong>
      ${isLastPass ? ' — <span style="color:var(--forest);font-weight:700;">✓ Sorted Output Verified</span>' : ''}
    `;
  }

  if (btnRunSort) {
    btnRunSort.addEventListener("click", async () => {
      const arr = parseNumberArray(sortArrInput.value);
      const algo = sortAlgoSelect.value;

      if (!arr.length) {
        alert("Please enter a valid array of numbers.");
        return;
      }

      btnRunSort.disabled = true;
      btnRunSort.textContent = "Sorting in C…";
      if (playInterval) clearInterval(playInterval);

      try {
        const resp = await fetch("/api/algorithms/sort", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ algorithm: algo, array: arr }),
        });
        const data = await resp.json();

        currentSortData = data;
        sortResultsArea.hidden = false;

        sortResultTitle.textContent = data.algorithm;
        sortComplexityNote.textContent = `Best: ${data.complexity.best_case} | Average: ${data.complexity.average_case} | Worst: ${data.complexity.worst_case} | Space: ${data.complexity.space_complexity}`;
        sortMetricCmp.textContent = data.comparisons;
        sortMetricSwaps.textContent = data.swaps;
        sortMetricShifts.textContent = data.shifts;
        sortMetricTime.textContent = data.time_microseconds.toFixed(2);

        renderPass(0);
      } catch (err) {
        alert("Sorting error: " + err.message);
      } finally {
        btnRunSort.disabled = false;
        btnRunSort.textContent = "Run C Sort";
      }
    });
  }

  if (btnPrevPass) {
    btnPrevPass.addEventListener("click", () => {
      if (playInterval) clearInterval(playInterval);
      renderPass(currentPassIdx - 1);
    });
  }

  if (btnNextPass) {
    btnNextPass.addEventListener("click", () => {
      if (playInterval) clearInterval(playInterval);
      renderPass(currentPassIdx + 1);
    });
  }

  if (btnPlayPasses) {
    btnPlayPasses.addEventListener("click", () => {
      if (playInterval) {
        clearInterval(playInterval);
        playInterval = null;
        btnPlayPasses.textContent = "▶ Auto Play";
      } else {
        if (!currentSortData || !currentSortData.passes.length) return;
        btnPlayPasses.textContent = "⏸ Pause";
        playInterval = setInterval(() => {
          if (currentPassIdx >= currentSortData.passes.length - 1) {
            clearInterval(playInterval);
            playInterval = null;
            btnPlayPasses.textContent = "▶ Replay";
          } else {
            renderPass(currentPassIdx + 1);
          }
        }, 550);
      }
    });
  }

  /* Preset Sorting Inputs */
  const btnPresetSorted = document.getElementById("btn-preset-sorted");
  const btnPresetReversed = document.getElementById("btn-preset-reversed");
  const btnPresetDups = document.getElementById("btn-preset-dups");

  if (btnPresetSorted) {
    btnPresetSorted.addEventListener("click", () => {
      sortArrInput.value = "10, 20, 30, 40, 50, 60, 70, 80";
    });
  }
  if (btnPresetReversed) {
    btnPresetReversed.addEventListener("click", () => {
      sortArrInput.value = "85, 70, 60, 45, 30, 25, 15, 5";
    });
  }
  if (btnPresetDups) {
    btnPresetDups.addEventListener("click", () => {
      sortArrInput.value = "25, 12, 45, 12, 88, 25, 12, 60";
    });
  }

  /* ----------------------------------------------------
     TAB 3: GREEDY VS DYNAMIC PROGRAMMING
  ---------------------------------------------------- */
  const optBudgetSlider = document.getElementById("opt-budget-slider");
  const budgetDisplay = document.getElementById("budget-display");
  const btnRunOpt = document.getElementById("btn-run-opt");
  const btnOptCounter = document.getElementById("btn-opt-counterexample");

  const greedySpent = document.getElementById("greedy-spent");
  const greedyRemaining = document.getElementById("greedy-remaining");
  const greedyScore = document.getElementById("greedy-score");
  const greedyTime = document.getElementById("greedy-time");
  const greedyExplanation = document.getElementById("greedy-explanation");
  const greedyItemsList = document.getElementById("greedy-items-list");

  const dpSpent = document.getElementById("dp-spent");
  const dpRemaining = document.getElementById("dp-remaining");
  const dpScore = document.getElementById("dp-score");
  const dpTime = document.getElementById("dp-time");
  const dpItemsList = document.getElementById("dp-items-list");

  const optAnalysisBox = document.getElementById("opt-analysis-box");
  const optAnalysisText = document.getElementById("opt-analysis-text");

  if (optBudgetSlider && budgetDisplay) {
    optBudgetSlider.addEventListener("input", () => {
      budgetDisplay.textContent = `৳${optBudgetSlider.value}`;
    });
  }

  async function executeOptimization(customPayload = null) {
    const budget = customPayload ? customPayload.budget : Number(optBudgetSlider.value);
    btnRunOpt.disabled = true;
    btnRunOpt.textContent = "Solving in C…";

    try {
      const payload = customPayload || { budget: budget };

      /* Run Greedy */
      const gResp = await fetch("/api/algorithms/greedy", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload),
      });
      const gData = await gResp.json();

      /* Run 0/1 Knapsack DP */
      const dpResp = await fetch("/api/algorithms/dynamic-programming/knapsack", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload),
      });
      const dpData = await dpResp.json();

      /* Render Greedy */
      greedySpent.textContent = `৳${gData.total_cost}`;
      greedyRemaining.textContent = `৳${gData.remaining_budget}`;
      greedyScore.textContent = gData.total_value;
      greedyTime.textContent = gData.time_microseconds.toFixed(2);
      greedyExplanation.textContent = gData.strategy_explanation;

      greedyItemsList.innerHTML = (gData.selected_items || [])
        .map(
          (item) => `
          <li class="picked-item">
            <span>${item.name} (৳${item.cost})</span>
            <span class="picked-item-val">+${item.utility_value} pts</span>
          </li>
        `
        )
        .join("") || '<li>No items could fit within this budget.</li>';

      /* Render DP */
      dpSpent.textContent = `৳${dpData.total_cost}`;
      dpRemaining.textContent = `৳${dpData.remaining_budget}`;
      dpScore.textContent = dpData.total_value;
      dpTime.textContent = dpData.time_microseconds.toFixed(2);

      dpItemsList.innerHTML = (dpData.selected_items || [])
        .map(
          (item) => `
          <li class="picked-item">
            <span>${item.name} (৳${item.cost})</span>
            <span class="picked-item-val">+${item.utility_value} pts</span>
          </li>
        `
        )
        .join("") || '<li>No items could fit within this budget.</li>';

      /* Comparison Analysis */
      optAnalysisBox.hidden = false;
      if (dpData.total_value > gData.total_value) {
        optAnalysisText.innerHTML = `
          <strong>Classic 0/1 Knapsack Counterexample Demonstrated!</strong><br />
          Dynamic Programming achieved <strong>${dpData.total_value}</strong> points, outperforming Greedy's <strong>${gData.total_value}</strong> points by <strong>${dpData.total_value - gData.total_value}</strong> points!<br />
          <em>Why did this happen?</em> The greedy heuristic picked the item with the highest density first, but its indivisible cost left unspendable budget. Dynamic Programming considered all combinatorial choices via the DP recurrence to find the true global maximum.
        `;
      } else if (dpData.total_value === gData.total_value) {
        optAnalysisText.innerHTML = `
          Both Greedy and Dynamic Programming achieved the same total utility (<strong>${dpData.total_value} pts</strong>). In this specific budget instance, the greedy heuristic happened to hit the optimal solution. Try clicking <em>"Load Suboptimality Proof Case"</em> to view where Greedy fails!
        `;
      }
    } catch (err) {
      alert("Optimization error: " + err.message);
    } finally {
      btnRunOpt.disabled = false;
      btnRunOpt.textContent = "Execute C Greedy & DP";
    }
  }

  if (btnRunOpt) {
    btnRunOpt.addEventListener("click", () => executeOptimization());
  }

  if (btnOptCounter) {
    btnOptCounter.addEventListener("click", () => {
      /* Known academic counterexample: Budget 10, items: 6(30), 5(20), 5(20) */
      const counterPayload = {
        budget: 10,
        items: [
          { id: 101, name: "Dense Single Textbook (Cost 6)", cost: 6, utility_value: 30 },
          { id: 102, name: "Lab Manual A (Cost 5)", cost: 5, utility_value: 20 },
          { id: 103, name: "Lab Manual B (Cost 5)", cost: 5, utility_value: 20 },
        ],
      };
      if (optBudgetSlider) optBudgetSlider.value = 10;
      if (budgetDisplay) budgetDisplay.textContent = "৳10 (Proof Preset)";
      executeOptimization(counterPayload);
    });
  }

  /* ----------------------------------------------------
     TAB 4: DIJKSTRA SHORTEST PATH
  ---------------------------------------------------- */
  const dijkstraSourceSelect = document.getElementById("dijkstra-source-select");
  const btnRunDijkstra = document.getElementById("btn-run-dijkstra");
  const dijkstraTbody = document.getElementById("dijkstra-tbody");
  const graphNodesMap = document.getElementById("graph-nodes-map");

  if (btnRunDijkstra) {
    btnRunDijkstra.addEventListener("click", async () => {
      const src = Number(dijkstraSourceSelect.value);
      btnRunDijkstra.disabled = true;
      btnRunDijkstra.textContent = "Computing Paths in C…";

      try {
        const resp = await fetch("/api/algorithms/graph/shortest-path", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ source_node: src }),
        });
        const data = await resp.json();

        dijkstraTbody.innerHTML = (data.locations || [])
          .map((loc) => {
            const isSrc = loc.id === data.source_node;
            const statusClass = loc.reachable ? "status-reachable" : "status-unreachable";
            const statusText = isSrc ? "Source Hub" : (loc.reachable ? "Reachable" : "UNREACHABLE");
            const distText = loc.reachable ? `${loc.distance_km} km` : "Infinity (∞)";
            const pathText = loc.reachable ? loc.path_indices.join(" ➔ ") : "None";

            return `
              <tr>
                <td><strong>Hub ${loc.id}:</strong> ${loc.name}</td>
                <td><strong>${distText}</strong></td>
                <td><code>${pathText}</code></td>
                <td><span class="${statusClass}">${statusText}</span></td>
              </tr>
            `;
          })
          .join("");

        /* Render node chips */
        if (graphNodesMap) {
          graphNodesMap.innerHTML = (data.locations || [])
            .map((loc) => {
              const isSrc = loc.id === data.source_node;
              let cls = "graph-node-chip";
              if (isSrc) cls += " is-source";
              if (!loc.reachable) cls += " is-unreachable";
              return `
                <div class="${cls}">
                  <span>[Hub ${loc.id}] ${loc.name}</span>
                  <span>${isSrc ? '★ SOURCE' : (loc.reachable ? `${loc.distance_km} km` : 'Disconnected Hub')}</span>
                </div>
              `;
            })
            .join("");
        }
      } catch (err) {
        alert("Dijkstra error: " + err.message);
      } finally {
        btnRunDijkstra.disabled = false;
        btnRunDijkstra.textContent = "Compute Shortest Paths in C";
      }
    });
  }

  /* ----------------------------------------------------
     TAB 5: FAIR BENCHMARK RUNNER
  ---------------------------------------------------- */
  const btnRunBenchmark = document.getElementById("btn-run-benchmark");
  const benchmarkSizeSelect = document.getElementById("benchmark-size-select");
  const benchmarkTarget = document.getElementById("benchmark-target");
  const benchmarkResults = document.getElementById("benchmark-results");
  const sortBenchmarkTbody = document.getElementById("sort-benchmark-tbody");
  const searchBenchmarkTbody = document.getElementById("search-benchmark-tbody");

  if (btnRunBenchmark) {
    btnRunBenchmark.addEventListener("click", async () => {
      const size = Number(benchmarkSizeSelect.value);
      const target = Number(benchmarkTarget.value);

      /* Generate a diverse deterministic random array of size N */
      const rndArray = Array.from({ length: size }, () => Math.floor(Math.random() * 200) + 1);

      btnRunBenchmark.disabled = true;
      btnRunBenchmark.textContent = "Benchmarking in C…";

      try {
        const resp = await fetch("/api/algorithms/benchmark", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ array: rndArray, target: target }),
        });
        const data = await resp.json();

        benchmarkResults.hidden = false;

        /* Sorting Table */
        sortBenchmarkTbody.innerHTML = (data.sorting_comparison || [])
          .map((row) => `
            <tr>
              <td><strong>${row.name}</strong></td>
              <td><code>${row.time_complexity}</code></td>
              <td>${row.space_complexity}</td>
              <td>${row.comparisons.toLocaleString()}</td>
              <td>${row.swaps.toLocaleString()}</td>
              <td>${row.shifts.toLocaleString()}</td>
              <td><strong>${row.time_microseconds.toFixed(2)}</strong></td>
              <td>${row.verified ? '✓ Verified' : '✗ Failed'}</td>
            </tr>
          `)
          .join("");

        /* Searching Table */
        searchBenchmarkTbody.innerHTML = (data.searching_comparison || [])
          .map((row) => {
            const isBin = row.name.includes("Binary");
            return `
              <tr>
                <td><strong>${row.name}</strong></td>
                <td>${isBin ? 'Sorted Copy of Array' : 'Original Unsorted Array'}</td>
                <td><code>${row.time_complexity}</code></td>
                <td>${row.comparisons.toLocaleString()}</td>
                <td>${row.found_index >= 0 ? `Index ${row.found_index}` : 'Not Found (-1)'}</td>
                <td><strong>${row.time_microseconds.toFixed(2)}</strong></td>
              </tr>
            `;
          })
          .join("");
      } catch (err) {
        alert("Benchmark error: " + err.message);
      } finally {
        btnRunBenchmark.disabled = false;
        btnRunBenchmark.textContent = "Execute Real-Time C Benchmark";
      }
    });
  }

  /* Auto-run initial tab */
  if (btnRunLinear) btnRunLinear.click();
})();
