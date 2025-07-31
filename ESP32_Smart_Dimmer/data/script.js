// Function to update UI based on light mode selection (from your original code)
function updateLightModeDisplay() {
    var lightMode = document.getElementById("lightMode").value;
    var mainLightGroup = document.getElementById("main-light");
    var rgbRingGroup = document.getElementById("rgb-ring");

    if (mainLightGroup && rgbRingGroup) { // Ensure elements exist
        switch (lightMode) {
            case "main":
                mainLightGroup.style.display = "block";
                rgbRingGroup.style.display = "none";
                break;
            case "rgb":
                mainLightGroup.style.display = "none";
                rgbRingGroup.style.display = "block";
                break;
            case "off":
            default: // Default to off if value is unexpected
                mainLightGroup.style.display = "none";
                rgbRingGroup.style.display = "none";
                break;
        }
    }
}

// Function to send control data (from your original code)
function sendControlData() {
    var lightMode = document.getElementById("lightMode").value;
    var brightness = document.getElementById("brightness").value;
    var warmness = document.getElementById("warmness").value;
    var rgbHue = document.getElementById("rgbHue").value;
    var rgbBrightness = document.getElementById("rgbBrightness").value;
    var fanSpeed = document.getElementById("fanSpeed").value;

    var xhr = new XMLHttpRequest();
    var url = "/control?" +
              "mode=" + lightMode +
              "&bright=" + brightness +
              "&warm=" + warmness +
              "&hue=" + rgbHue +
              "&rgbBright=" + rgbBrightness +
              "&fan=" + fanSpeed;

    xhr.open("GET", url, true);
    xhr.onreadystatechange = function () {
        var responseDiv = document.getElementById("response");
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                responseDiv.className = "show success"; // Added 'success' class
                responseDiv.innerText = "Status: " + xhr.responseText;
            } else {
                responseDiv.className = "show error"; // Added 'error' class
                responseDiv.innerText = "Error: " + xhr.status + " " + xhr.statusText;
            }
            // Hide response after 3 seconds
            setTimeout(() => { responseDiv.className = ""; }, 3000);
        }
    };
    xhr.send();
}

// Functions for device selection dialog (from your original code, adapted for new modal structure)
var deviceSelectionOverlay = document.getElementById("device-selection-overlay");

function openDeviceSelectionDialog() {
    deviceSelectionOverlay.classList.add("show");
    // In the next step, we'll add search logic here
}

function closeDeviceSelectionDialog() {
    deviceSelectionOverlay.classList.remove("show");
}

function onDeviceSelectionDialogCancel() {
    closeDeviceSelectionDialog();
}


// Event Listeners (Instead of onclick/onchange in HTML)
document.addEventListener('DOMContentLoaded', () => {
    // Initial display update when page loads
    updateLightModeDisplay();

    // Attach listeners for control changes
    document.getElementById('lightMode').addEventListener('change', () => {
        updateLightModeDisplay();
        sendControlData(); // Uncomment this line when you're ready to send data
    });
    document.getElementById('brightness').addEventListener('change', sendControlData);
    document.getElementById('warmness').addEventListener('change', sendControlData);
    document.getElementById('rgbHue').addEventListener('change', sendControlData);
    document.getElementById('rgbBrightness').addEventListener('change', sendControlData);
    document.getElementById('fanSpeed').addEventListener('change', sendControlData);

    // Attach listener for add device button
    document.getElementById('add-device').addEventListener('click', openDeviceSelectionDialog);

    // Attach listener for modal cancel button
    document.getElementById('cancel-device-selection').addEventListener('click', onDeviceSelectionDialogCancel);

    // Initial update for range input fill (visual feedback)
    document.querySelectorAll('input[type="range"]').forEach(input => {
        const updateRangeFill = () => {
            const min = parseFloat(input.min);
            const max = parseFloat(input.max);
            const value = parseFloat(input.value);
            const percent = ((value - min) / (max - min)) * 100;
            input.style.setProperty('--fill-percent', `${percent}%`);
        };
        input.addEventListener('input', updateRangeFill);
        updateRangeFill(); // Call on load to set initial state
    });
});