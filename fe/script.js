// Inicializace proměnných
let chart = null;
let fetchInterval = null;
let originalData = [];
let currentTransformFn = (x) => x; // Výchozí transformační funkce (identita)
let currentAxisLabel = "Value"; // Výchozí popisek osy
const statusElement = document.getElementById("status");
const startBtn = document.getElementById("startBtn");
const stopBtn = document.getElementById("stopBtn");
const clearBtn = document.getElementById("clearBtn");
const configBtn = document.getElementById("configBtn");
const exportBtn = document.getElementById("exportBtn");
const resetZoomBtn = document.getElementById("resetZoomBtn");
const applyTransformBtn = document.getElementById("applyTransformBtn");

const startValueInput = document.getElementById("startValue");
const stopValueInput = document.getElementById("stopValue");
const stepValueInput = document.getElementById("stepValue");
const countPeriodInput = document.getElementById("countPeriod");
const xAxisFunctionInput = document.getElementById("xAxisFunction");
const functionLabelInput = document.getElementById("functionLabel");

// Základní URL pro API
const API_BASE_URL = "http://127.0.0.1:8000";

// Pomocná funkce pro volání API
async function callApi(endpoint, method = "GET", data = null) {
    try {
        const options = {
            method: method,
            headers: {
                "Content-Type": "application/json",
                Accept: "application/json",
            },
            mode: "cors",
            credentials: "omit",
        };

        if (data && (method === "POST" || method === "PUT")) {
            options.body = JSON.stringify(data);
        }

        console.log(`Volám API: ${method} ${API_BASE_URL}${endpoint}`, options);
        const response = await fetch(`${API_BASE_URL}${endpoint}`, options);

        if (!response.ok) {
            console.error(`API chyba: ${response.status} ${response.statusText}`);
            const errorText = await response.text();
            console.error(`Detaily chyby:`, errorText);
            throw new Error(`HTTP error! Status: ${response.status} ${response.statusText}`);
        }

        const result = await response.json();
        console.log(`API odpověď:`, result);
        return result;
    } catch (error) {
        console.error(`API volání selhalo:`, error);
        throw error;
    }
}

// Inicializace grafu
function initChart() {
    const ctx = document.getElementById("histogramChart").getContext("2d");
    chart = new Chart(ctx, {
        type: "bar",
        data: {
            labels: [],
            datasets: [
                {
                    label: "Histogram",
                    data: [],
                    backgroundColor: "rgba(54, 162, 235, 0.5)",
                    borderColor: "rgba(54, 162, 235, 1)",
                    borderWidth: 1,
                },
            ],
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: true,
                    title: {
                        display: true,
                        text: "Count",
                    },
                },
                x: {
                    title: {
                        display: true,
                        text: "Value",
                    },
                },
            },
            plugins: {
                zoom: {
                    pan: {
                        enabled: true,
                        mode: "x",
                    },
                    zoom: {
                        wheel: {
                            enabled: true,
                        },
                        pinch: {
                            enabled: true,
                        },
                        mode: "x",
                        drag: {
                            enabled: true,
                            backgroundColor: "rgba(54, 162, 235, 0.2)",
                            borderColor: "rgba(54, 162, 235, 0.5)",
                        },
                    },
                },
                tooltip: {
                    callbacks: {
                        label: function (context) {
                            return `Count: ${context.raw}`;
                        },
                        title: function (context) {
                            return `Value: ${context[0].label}`;
                        },
                    },
                },
            },
        },
    });
}

// Funkce pro načtení dat
// ... existing code ...
async function fetchData() {
    try {
        statusElement.textContent = "Fetching data...";
        const newData = await callApi("/");

        // Pokud ještě nemáme žádná data, použijeme nová data jako výchozí
        if (!originalData || originalData.length === 0) {
            originalData = [...newData];
        } else {
            // Agregace nových dat s existujícími
            // Pro každou hodnotu v nových datech
            newData.forEach(newItem => {
                const [newValue, newCount] = newItem;

                // Zkontrolujeme, zda tato hodnota již existuje v původních datech
                const existingItemIndex = originalData.findIndex(item => item[0] === newValue);

                if (existingItemIndex >= 0) {
                    // Pokud hodnota existuje, přičteme počet
                    originalData[existingItemIndex][1] += newCount;
                } else {
                    // Pokud hodnota neexistuje, přidáme nový záznam
                    originalData.push([newValue, newCount]);
                }
            });

            // Seřadíme data podle hodnoty pro konzistentní zobrazení
            originalData.sort((a, b) => a[0] - b[0]);
        }

        // Aplikace aktuální transformační funkce na agregovaná data
        const transformedData = applyCurrentTransformation(originalData);
        updateChart(transformedData);

        const now = new Date();
        statusElement.textContent = `Data updated (cumulative): ${now.toLocaleTimeString()}`;
    } catch (error) {
        statusElement.textContent = `Error: ${error.message}`;
        console.error("Fetch error:", error);
    }
}

// Aplikace aktuální transformační funkce na data
function applyCurrentTransformation(data) {
    if (!data || data.length === 0) return [];

    return data.map((item) => {
        const x = item[0];
        try {
            const transformedX = currentTransformFn(x);
            return [transformedX, item[1]];
        } catch (e) {
            console.error(`Chyba při transformaci hodnoty ${x}:`, e);
            return item; // Při chybě ponechat původní hodnotu
        }
    });
}

// Aplikace transformační funkce na osu X
function applyTransformation() {
    if (!originalData || originalData.length === 0) {
        statusElement.textContent = "No data to transform";
        return;
    }

    try {
        const functionStr = xAxisFunctionInput.value.trim() || "x";
        const axisLabel = functionLabelInput.value.trim() || "Value";

        // Vytvoření funkce z řetězce
        const transformFn = new Function("x", `return ${functionStr}`);

        // Test funkce s hodnotou 1 pro ověření
        transformFn(1);

        // Uložení aktuální transformační funkce a popisku
        currentTransformFn = transformFn;
        currentAxisLabel = axisLabel;

        // Transformace dat
        const transformedData = applyCurrentTransformation(originalData);

        // Aktualizace grafu
        updateChart(transformedData);

        // Aktualizace popisku osy X
        chart.options.scales.x.title.text = axisLabel;
        chart.update();

        statusElement.textContent = `Transformation applied: ${functionStr}`;
    } catch (error) {
        statusElement.textContent = `Transformation error: ${error.message}`;
        console.error("Transformation error:", error);
        alert(`Invalid function: ${error.message}`);
    }
}

// Aktualizace grafu s novými daty
function updateChart(data) {
    if (!chart) {
        initChart();
    }

    // Nyní předpokládáme, že data jsou 2D pole [[hodnota, počet], ...]
    const labels = data.map((item) => Number(item[0]).toFixed(2));
    const counts = data.map((item) => item[1]);

    chart.data.labels = labels;
    chart.data.datasets[0].data = counts;
    chart.update();
}

// Odeslání konfigurace na server
async function sendConfig() {
    try {
        const startValue = parseFloat(startValueInput.value);
        const stopValue = parseFloat(stopValueInput.value);
        const stepValue = parseFloat(stepValueInput.value);

        if (startValue >= stopValue) {
            alert("Start value must be less than stop value");
            return;
        }

        if (stepValue <= 0) {
            alert("Step value must be greater than 0");
            return;
        }

        // Zaokrouhlení na 2 desetinná místa pro konzistenci
        const data = {
            start_value: parseFloat(Number(startValue).toFixed(2)),
            stop_value: parseFloat(Number(stopValue).toFixed(2)),
            step_value: parseFloat(Number(stepValue).toFixed(2)),
        };

        const result = await callApi("/config", "POST", data);
        statusElement.textContent = `Configuration updated: Start=${result.start_value}, Stop=${result.stop_value}, Step=${result.step_value}`;
    } catch (error) {
        statusElement.textContent = `Config error: ${error.message}`;
        console.error("Config error:", error);
    }
}

// Spuštění generování dat
async function startGeneration() {
    try {
        const countPeriod = parseInt(countPeriodInput.value);

        if (countPeriod <= 0) {
            alert("Update period must be greater than 0");
            return;
        }

        const data = {
            count_period: countPeriod,
        };

        const result = await callApi("/start", "POST", data);
        statusElement.textContent = `Generation started with period: ${result.count_period}s`;

        // Nastav UI
        startBtn.disabled = true;
        stopBtn.disabled = false;
        configBtn.disabled = true;
        clearBtn.disabled = true;
        exportBtn.disabled = true;

        // Spusť periodické načítání dat
        fetchData(); // Okamžité načtení
        fetchInterval = setInterval(fetchData, countPeriod * 1000); // Aktualizuj každou sekundu
    } catch (error) {
        statusElement.textContent = `Start error: ${error.message}`;
        console.error("Start error:", error);
    }
}

// Zastavení generování dat
async function stopGeneration() {
    try {
        const result = await callApi("/stop", "POST");
        statusElement.textContent = `Generation stopped`;

        // Nastav UI
        startBtn.disabled = false;
        stopBtn.disabled = true;
        configBtn.disabled = false;
        clearBtn.disabled = false;
        exportBtn.disabled = false;

        // Zastav periodické načítání
        if (fetchInterval) {
            clearInterval(fetchInterval);
            fetchInterval = null;
        }
    } catch (error) {
        statusElement.textContent = `Stop error: ${error.message}`;
        console.error("Stop error:", error);
    }
}

// Vyčištění dat
async function clearData() {
    try {
        const result = await callApi("/clear", "POST");
        statusElement.textContent = `Data cleared`;
        originalData = [];
        // Aktualizuj graf
        fetchData();
    } catch (error) {
        statusElement.textContent = `Clear error: ${error.message}`;
        console.error("Clear error:", error);
    }
}

// Funkce pro export dat do CSV
function exportToCsv() {
    if (!chart || !chart.data.labels.length) {
        alert("No data to export");
        return;
    }

    // Vytvoření CSV obsahu
    const csvContent = "data:text/csv;charset=utf-8,";
    const header = "Value,Count\n";

    const rows = chart.data.labels
        .map((label, index) => {
            const count = chart.data.datasets[0].data[index];
            return `${label},${count}`;
        })
        .join("\n");

    const csvData = csvContent + header + rows;

    // Vytvoření odkazu pro stažení
    const encodedUri = encodeURI(csvData);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute(
        "download",
        `histogram_data_${new Date().toISOString().slice(0, 19).replace(/:/g, "-")}.csv`
    );
    document.body.appendChild(link);

    // Kliknutí na odkaz a jeho odstranění
    link.click();
    document.body.removeChild(link);

    statusElement.textContent = "Data exported to CSV";
}

// Funkce pro reset zoomu
function resetZoom() {
    if (chart) {
        chart.resetZoom();
        statusElement.textContent = "Zoom reset";
    }
}

// Event listenery
startBtn.addEventListener("click", startGeneration);
stopBtn.addEventListener("click", stopGeneration);
clearBtn.addEventListener("click", clearData);
configBtn.addEventListener("click", sendConfig);
exportBtn.addEventListener("click", exportToCsv);
resetZoomBtn.addEventListener("click", resetZoom);
applyTransformBtn.addEventListener("click", applyTransformation);

// Inicializace
initChart();
fetchData(); // Načti počáteční data 