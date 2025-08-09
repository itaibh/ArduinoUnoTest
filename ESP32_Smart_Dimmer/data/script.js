// Helper to get element by ID safely
const getById = (id) => document.getElementById(id);

let rgbPickerInitialized = false;
let warmnessIntensityPickerInitialized = false;
let registeredDevices = {};

let warmnessPickerImage = getById('warmness-picker-image');
let warmnessPointer = getById('warmness-pointer');
let intensityValueInput = getById('intensityValue');
let warmnessValueInput = getById('warmnessValue');

let colorPickerImage = getById('color-picker-image');
let huePointer = getById('hue-pointer');
let rgbHueInput = getById('rgbHue');
let rgbValueInput = getById('rgbValue');

let searchForDevicesBtn = getById('search-devices');
let searchIndicator = getById('search-indicator');

// Function to update UI based on light mode selection
function updateLightModeDisplay() {
    const lightMode = getById("lightMode").value;
    const mainLightGroup = getById("main-light");
    const rgbRingGroup = getById("rgb-ring");

    if (mainLightGroup && rgbRingGroup) { // Ensure elements exist
        if (lightMode === "main") {
            mainLightGroup.style.display = "block";
            rgbRingGroup.style.display = "none";
            if (!warmnessIntensityPickerInitialized) {
                initWarmnessIntensityPicker();
                warmnessIntensityPickerInitialized = true;
            }
            // Update pointer to current value when switching modes
            updateWarmnessIntensityPointer(parseInt(warmnessValueInput.value), parseInt(intensityValueInput.value));
        } else if (lightMode === "rgb") {
            mainLightGroup.style.display = "none";
            rgbRingGroup.style.display = "block";
            if (!rgbPickerInitialized) {
                initRgbPicker();
                rgbPickerInitialized = true;
            }
            // Update pointer to current value when switching modes
            updateRgbPointer(parseInt(rgbHueInput.value), parseInt(rgbValueInput.value));
        } else { // lightMode === "off"
            mainLightGroup.style.display = "none";
            rgbRingGroup.style.display = "none";
        }
    }
}

const lightModeInput = getById("lightMode");
const fanSpeedInput = getById("fanSpeed");
// Function to send control data (reads from updated elements)
function sendControlData() {
    const lightMode = lightModeInput.value;
    const mac = deviceControlDiv.dataset.mac;
    const device = registeredDevices[mac];
    let url = "/control?address=" + mac + "&";

    if (lightMode === "main") {
        const intensity = intensityValueInput.value; // 1-16
        const warmness = warmnessValueInput.value;   // 0-255
        url += `mode=${lightMode}&bright=${intensity}&warm=${warmness}`;
        device.light_mode = "main"
        device.main_brightness = intensity
        device.main_warmness = warmness
        device.is_on = true
    } else if (lightMode === "rgb") {
        const rgbHue = rgbHueInput.value;     // 0-100
        const rgbValue = rgbValueInput.value; // 0-255 (brightness for RGB)
        url += `mode=${lightMode}&hue=${rgbHue}&rgbValue=${rgbValue}`;
        device.light_mode = "rgb"
        device.ring_hue = rgbHue;
        device.ring_brightness = rgbValue
        device.is_on = true
    } else { // Off mode
        url += `mode=${lightMode}`;
        device.is_on = false
    }

    const fanSpeed = fanSpeedInput.value;
    device.fan_speed = fanSpeed
    url += `&fan=${fanSpeed}`;
    registeredDevices[mac] = device;
    const deviceInfoDiv = getById(`device-${mac.replace(/:/g, '')}`);
    const lightStatusSpan = deviceInfoDiv.querySelector(".device-status>.light-status>span.status");
    const fanStatusSpan = deviceInfoDiv.querySelector(".device-status>.fan-status>span.status");
    lightStatusSpan.innerHTML = device.is_on ? lightMode : "off";
    lightStatusSpan.className = device.is_on ? "status status-on" : "status staus-off";
    fanStatusSpan.innerHTML = fanSpeeds[device.fan_speed];

    performGet(url);
}

function performGet(url, callback) {
    const xhr = new XMLHttpRequest();
    xhr.open("GET", url, true);
    xhr.onreadystatechange = function () {
        const responseDiv = getById("response");
        if (xhr.readyState === 4) {
            if (xhr.status === 200) {
                responseDiv.className = "show success";
                if (callback) {
                    callback(xhr.responseText);
                } else {
                    // responseDiv.innerText = "Status: " + xhr.responseText;
                }
            } else {
                responseDiv.className = "show error";
                responseDiv.innerText = "Error: " + xhr.status + " " + xhr.statusText;
            }
            setTimeout(() => { responseDiv.className = ""; }, 3000);
        }
    };
    // Uncomment this line when ready to send actual data to your ESP32
    xhr.send(); // Sending actual data now
    console.log("Sending control data:", url);
}

// --- Device Selection Dialog Functions (no change) ---
const deviceSelectionOverlay = getById("device-selection-overlay");

function openDeviceSelectionDialog() {
    deviceSelectionOverlay.classList.add("show");
    searchForDevices();
}

function closeDeviceSelectionDialog() {
    deviceSelectionOverlay.classList.remove("show");
}

const deviceListContainer = getById("device-list-container");
function searchForDevices() {
    let url = "/discover_devices";
    searchForDevicesBtn.setAttribute("disabled", "disabled");
    searchIndicator.style.display = "block";
    performGet(url, (responseText => {
        try {
            const devices = JSON.parse(responseText);
            displayDiscoveredDevices(devices);
        } catch (e) {
            // Handle JSON parsing errors specifically
            deviceListContainer.innerHTML = '<div class="error-message">Error parsing device data.</div>';
            console.error("Error parsing JSON response:", e);
        }
        searchForDevicesBtn.removeAttribute("disabled");
        searchIndicator.style.display = "none";
    }));
}

const mainPage = getById("main");
const registeredDevicesDiv = getById("registered-devices");
const noDevicesSpan = getById("no-devices");
const deviceControlDiv = getById("device-control");
const currentDeviceTitleHeader = deviceControlDiv.querySelector(".title > h1");
const fanSpeeds = ["Off", "Low", "Medium", "High"];
function reloadMainPage() {
    // Clear previous content to avoid duplicates on reload
    registeredDevicesDiv.innerHTML = "";

    // Optional: Show a loading message while fetching data
    registeredDevicesDiv.innerHTML = "<p>Loading configured devices...</p>";

    // Fetch the list of registered devices from the mock server (or ESP32)
    // Using /get_all_devices for the mock server as per your Python code
    // If on ESP32, this would be /get_managed_devices
    performGet("/get_all_devices", (response) => {
        // Clear the loading message once data is received
        registeredDevicesDiv.innerHTML = "";

        const devices = JSON.parse(response)
        registeredDevices = devices;
        if (devices.length === 0) {
            // If no devices are registered, show the "No devices configured yet." message
            noDevicesSpan.style.display = "block";
        } else {
            // If devices are found, hide the "No devices" message
            noDevicesSpan.style.display = "none";

            // Iterate over each device and create its HTML representation
            Object.values(devices).forEach(device => {
                const deviceElement = document.createElement("div");
                deviceElement.className = "registered-device-item"; // Add a class for CSS styling
                // Give a unique ID for potential future targeting (e.g., controlling a specific device)
                deviceElement.id = `device-${device.mac_address.replace(/:/g, '')}`;

                // Determine ON/OFF status and apply a class for styling
                const isLightOnStatus = device.is_on ? "ON" : "OFF";
                const lightMode = device.is_on ? device.light_mode : "off";
                const lightStatusClass = device.is_on ? "status-on" : "status-off";
                const fanStatus = fanSpeeds[device.fan_speed];

                deviceElement.innerHTML = `
                    <div class="data" data-mac="${device.mac_address}" data-name="${device.name}">
                        <h3>${device.name || "Unnamed Device"}</h3>
                        <p>MAC: ${device.mac_address}</p>

                        <div class="device-status">
                            <div class="light-status">
                                <span class="status ${lightStatusClass}">${lightMode}</span>
                            </div>
                            <div class="fan-status">
                                <span class="status">${fanStatus}</span>
                            </div>
                        </div>
                    </div>
                    <div class="device-actions">
                        <button class="remove-device-btn" data-mac="${device.mac_address}"><span class="button-text">Remove</span></button>
                    </div>
                `;
                registeredDevicesDiv.appendChild(deviceElement);

                deviceElement.querySelector(".data").addEventListener("click", (e) => {
                    const mac = e.currentTarget.dataset.mac;
                    const name = e.currentTarget.dataset.name;
                    goToDeviceControlPage(mac, name);
                });

                deviceElement.querySelector(".remove-device-btn").addEventListener("click", (e) => {
                    const mac = e.currentTarget.dataset.mac;
                    console.log(`Remove button clicked for MAC: ${mac}`);
                    if (confirm(`Are you sure you want to remove device ${mac}?`)) {
                        performGet(`/remove_device?address=${mac}`, () => reloadMainPage())
                    }
                });
            });
        }
    });
}

function goToDeviceControlPage(mac, name) {
    console.log(`Control button clicked for MAC: ${mac} (${name})`);
    deviceControlDiv.style.display = "block";
    mainPage.style.display = "none";
    deviceControlDiv.dataset.mac = mac;
    deviceControlDiv.dataset.name = name;
    const device = registeredDevices[mac]
    const isOn = device.is_on;
    lightModeInput.value = isOn ? device.light_mode : "off";
    fanSpeedInput.value = device.fan_speed;
    warmnessValueInput.value = device.main_warmness;
    intensityValueInput.value = device.main_brightness;
    rgbHueInput.value = device.ring_hue;
    rgbValueInput.value = device.ring_brightness;
    currentDeviceTitleHeader.innerHTML = name;
    updateLightModeDisplay();
}

function goHome() {
    mainPage.style.display = "block";
    deviceControlDiv.style.display = "none";
}

function addDevice(device) {
    alert(`Selected device: ${device.name} (MAC: ${device.mac_address})`);
    performGet(`/add_device?name=${device.name}&address=${device.mac_address}`);
    reloadMainPage();
}

function displayDiscoveredDevices(devices) {
    deviceListContainer.innerHTML = ''; // Clear previous content

    if (devices.length === 0) {
        deviceListContainer.innerHTML = '<div class="info-message">No devices found.</div>';
        return;
    }

    devices.forEach(device => {
        const deviceItem = document.createElement('div');
        deviceItem.className = 'device-item';
        deviceItem.innerHTML = `
            <strong>${device.name}</strong><br>
            <small>${device.mac_address}</small>
        `;
        deviceItem.addEventListener('click', () => {
            addDevice(device);
            // Store selected device info or update main UI here
            closeDeviceSelectionDialog();
        });
        deviceListContainer.appendChild(deviceItem);
    });
}
// --- HSL/RGB Color Conversion Helpers (Needed for both pickers) ---

/**
 * Converts an HSL color value to RGB. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes h, s, and l are contained in the set [0, 1] and
 * returns r, g, and b in the set [0, 255].
 *
 * @param   {number}  h       The hue
 * @param   {number}  s       The saturation
 * @param   {number}  l       The lightness
 * @return  {Array<number>}   The RGB representation
 */
function hslToRgb(h, s, l) {
    let r, g, b;

    if (s === 0) {
        r = g = b = l; // achromatic
    } else {
        const hue2rgb = (p, q, t) => {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1 / 6) return p + (q - p) * 6 * t;
            if (t < 1 / 2) return q;
            if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
            return p;
        };

        const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        const p = 2 * l - q;
        r = hue2rgb(p, q, h + 1 / 3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1 / 3);
    }

    return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];
}

// Linear interpolation for colors
function lerpColor(color1, color2, t) {
    return [
        Math.round(color1[0] + (color2[0] - color1[0]) * t),
        Math.round(color1[1] + (color2[1] - color1[1]) * t),
        Math.round(color1[2] + (color2[2] - color1[2]) * t)
    ];
}


// --- RGB Ring Color Picker Logic (Mathematical) ---
let isDraggingRgb = false;

// Define the radial gradient properties for mathematical calculation
const _RGB_INNER_BLACK_RADIUS_RATIO = 0.0; // Black center ratio
const _RGB_HUE_RING_RADIUS_RATIO = 0.5;    // Fully saturated hue ring ratio
const _RGB_OUTER_RADIUS_RATIO = 1.0;       // Full outer radius (white)

function initRgbPicker() {
    // No need for offscreen canvas setup anymore
    console.log("RGB Color Picker Initialized (Mathematical).");

    colorPickerImage.addEventListener('mousedown', startRgbDrag);
    colorPickerImage.addEventListener('mousemove', dragRgb);
    colorPickerImage.addEventListener('mouseup', endRgbDrag);
    colorPickerImage.addEventListener('mouseleave', endRgbDrag); // Stop dragging if mouse leaves picker area

    colorPickerImage.addEventListener('touchstart', startRgbDrag, { passive: false });
    colorPickerImage.addEventListener('touchmove', dragRgb, { passive: false });
    colorPickerImage.addEventListener('touchend', endRgbDrag);

    // Set initial pointer position based on current input values
    updateRgbPointer(parseInt(rgbHueInput.value), parseInt(rgbValueInput.value));
}

function getRgbColorAndValuesFromCoords(event) {
    const rect = colorPickerImage.getBoundingClientRect();
    const centerX = rect.width / 2;
    const centerY = rect.height / 2;
    const outerRadius = rect.width / 2; // Assuming square picker for radius

    const clientX = event.clientX || (event.touches ? event.touches[0].clientX : 0);
    const clientY = event.clientY || (event.touches ? event.touches[0].clientY : 0);

    // Calculate coordinates relative to the center of the image
    const x = clientX - rect.left - centerX;
    const y = clientY - rect.top - centerY;

    const distance = Math.sqrt(x * x + y * y);
    // Ensure distance doesn't exceed outer radius for calculations
    const clampedDistance = Math.min(distance, outerRadius);

    // Calculate Hue (angle)
    // atan2 gives radians from -PI to PI. Convert to 0-360 degrees.
    // Adjust by +90 to make 0 degrees point to the right (Red).
    let angleDeg = (Math.atan2(y, x) * 180 / Math.PI + 360) % 360;
    // Map angle 0-360 to rgbHue 0-100 (API value)
    const uiHue = Math.round(angleDeg / 3.6);


    // Calculate Lightness (L) and Saturation (S) based on radial distance
    const innerBlackRadius = outerRadius * _RGB_INNER_BLACK_RADIUS_RATIO;
    const hueRingRadius = outerRadius * _RGB_HUE_RING_RADIUS_RATIO;
    const outerWhiteRadius = outerRadius * _RGB_OUTER_RADIUS_RATIO;

    let l_norm, s_norm; // HSL values (0-1)

    if (clampedDistance <= innerBlackRadius) {
        // Pure black in the center
        l_norm = 0;
        s_norm = 0;
    } else if (clampedDistance <= hueRingRadius) {
        // Transition from black to full hue
        const progress = (clampedDistance - innerBlackRadius) / (hueRingRadius - innerBlackRadius);
        l_norm = progress * 0.5; // L from 0 to 0.5
        s_norm = progress;       // S from 0 to 1
    } else {
        // Transition from full hue to white
        const progress = (clampedDistance - hueRingRadius) / (outerWhiteRadius - hueRingRadius);
        l_norm = 0.5 + progress * 0.5; // L from 0.5 to 1
        s_norm = 1 - progress;       // S from 1 to 0
    }

    // Map L (0-1) to rgbValue (0-255) for API brightness
    const uiValue = Math.round(l_norm * 255);

    // Get RGB for pointer color based on the calculated HSL
    const pointerRgbColor = hslToRgb(angleDeg / 360, s_norm, l_norm); // HSL takes h in 0-1 range

    return { uiHue, uiValue, pointerRgbColor, pointerX: x, pointerY: y, clampedDistance };
}

function updateRgbPointer(uiHueValue, uiValueValue) {
    // If called with values from API/inputs, we need to convert them back to (x,y)
    // uiHueValue (0-100) -> angle (0-360)
    const angleDeg = uiHueValue * 3.6;
    const angleRad = (angleDeg) * Math.PI / 180; // No -90 offset here, let's keep it consistent

    // uiValueValue (0-255) -> L (0-1)
    const l_norm = uiValueValue / 255;

    // Inverse calculation of distance based on L for pointer position
    const imgRect = colorPickerImage.getBoundingClientRect();
    const outerRadius = imgRect.width / 2;
    const innerBlackRadius = outerRadius * _RGB_INNER_BLACK_RADIUS_RATIO;
    const hueRingRadius = outerRadius * _RGB_HUE_RING_RADIUS_RATIO;
    const outerWhiteRadius = outerRadius * _RGB_OUTER_RADIUS_RATIO;

    let distFromCenter;
    let s_norm_for_color;

    if (l_norm <= 0.5) {
        // From black to hue ring
        s_norm_for_color = l_norm * 2; // S from 0 to 1
        const progress = l_norm * 2; // L from 0 to 0.5 (progress 0 to 1)
        distFromCenter = innerBlackRadius + progress * (hueRingRadius - innerBlackRadius);
    } else {
        // From hue ring to white
        s_norm_for_color = 1 - (l_norm - 0.5) * 2; // S from 1 to 0
        const progress = (l_norm - 0.5) * 2; // L from 0.5 to 1 (progress 0 to 1)
        distFromCenter = hueRingRadius + progress * (outerWhiteRadius - hueRingRadius);
    }

    // Clamp distance to ensure pointer stays within the image bounds visually
    distFromCenter = Math.min(distFromCenter, outerRadius);

    // Calculate pointer's (x,y) position relative to image center
    const pointerX = distFromCenter * Math.cos(angleRad);
    const pointerY = distFromCenter * Math.sin(angleRad);

    // Set pointer CSS position
    huePointer.style.left = `${imgRect.width / 2 + pointerX}px`;
    huePointer.style.top = `${imgRect.height / 2 + pointerY}px`;

    // Set pointer color using the derived HSL for display
    const [r, g, b] = hslToRgb(angleDeg / 360, s_norm_for_color, l_norm);
    huePointer.style.backgroundColor = `rgb(${r}, ${g}, ${b})`;

    // Update hidden input values
    rgbHueInput.value = uiHueValue;
    rgbValueInput.value = uiValueValue;
}


function startRgbDrag(e) {
    e.preventDefault();
    isDraggingRgb = true;
    const { uiHue, uiValue, pointerRgbColor } = getRgbColorAndValuesFromCoords(e);
    updateRgbPointer(uiHue, uiValue); // Updates position and color
    sendControlData();
}

function dragRgb(e) {
    if (!isDraggingRgb) return;
    const { uiHue, uiValue, pointerRgbColor } = getRgbColorAndValuesFromCoords(e);
    updateRgbPointer(uiHue, uiValue); // Updates position and color
    sendControlData();
}

function endRgbDrag() {
    isDraggingRgb = false;
}


// --- Main Light Warmness/Intensity Picker Logic (Mathematical) ---
let isDraggingWarmnessIntensity = false;

// Define RGB values for the ends of the warmness spectrum
const COOL_WHITE_RGB = [255, 255, 255]; // #FFFFFF
const WARM_YELLOW_RGB = [255, 241, 118]; // #FFF176F

function initWarmnessIntensityPicker() {
    // No need for offscreen canvas setup anymore
    console.log("Warmness/Intensity Picker Initialized (Mathematical).");

    warmnessPickerImage.addEventListener('mousedown', startWarmnessIntensityDrag);
    warmnessPickerImage.addEventListener('mousemove', dragWarmnessIntensity);
    warmnessPickerImage.addEventListener('mouseup', endWarmnessIntensityDrag);
    warmnessPickerImage.addEventListener('mouseleave', endWarmnessIntensityDrag); // Stop dragging if mouse leaves picker area

    warmnessPickerImage.addEventListener('touchstart', startWarmnessIntensityDrag, { passive: false });
    warmnessPickerImage.addEventListener('touchmove', dragWarmnessIntensity, { passive: false });
    warmnessPickerImage.addEventListener('touchend', endWarmnessIntensityDrag);

    // Set initial pointer position based on current input values
    updateWarmnessIntensityPointer(parseInt(warmnessValueInput.value), parseInt(intensityValueInput.value));
}

function getWarmnessIntensityAndColorFromCoords(event) {
    const rect = warmnessPickerImage.getBoundingClientRect();
    const clientX = event.clientX || (event.touches ? event.touches[0].clientX : 0);
    const clientY = event.clientY || (event.touches ? event.touches[0].clientY : 0);

    // Calculate coordinates relative to the image's top-left corner
    const x = clientX - rect.left;
    const y = clientY - rect.top;

    // Clamp coordinates to image dimensions
    const clampedX = Math.max(0, Math.min(x, rect.width));
    const clampedY = Math.max(0, Math.min(y, rect.height));

    // Calculate warmness (0-255) from X position
    // Map X (0 to rect.width) to warmness (0-255)
    const warmness = Math.round((clampedX / rect.width) * 255);

    // Calculate intensity (1-16) from Y position
    // Map Y (0 to rect.height) to brightness (inverted: 1 at bottom, 16 at top)
    const brightnessRatio = (rect.height - clampedY) / rect.height; // 0 at bottom, 1 at top
    const intensity = Math.max(1, Math.min(16, Math.round(brightnessRatio * 15) + 1)); // Scale 0-1 to 1-16


    // Calculate pointer's RGB color
    // 1. Get the color from the top row (warmness gradient)
    const warmnessProgress = clampedX / rect.width; // 0 to 1
    const topRowColor = lerpColor(COOL_WHITE_RGB, WARM_YELLOW_RGB, warmnessProgress);

    // 2. Interpolate from the top row color down to black based on Y position
    const brightnessProgress = clampedY / rect.height; // 0 at top, 1 at bottom
    const pointerRgbColor = lerpColor(topRowColor, [0, 0, 0], brightnessProgress); // Lerp from top color to black

    return { warmness, intensity, pointerRgbColor, pointerX: clampedX, pointerY: clampedY };
}

function updateWarmnessIntensityPointer(warmnessValue, intensityValue) {
    // Re-calculate pointer X, Y based on given warmness and intensity values
    const imgRect = warmnessPickerImage.getBoundingClientRect();
    const imgWidth = imgRect.width;
    const imgHeight = imgRect.height;

    // Calculate X position from warmness (0-255)
    const x = (warmnessValue / 255) * imgWidth;

    // Calculate Y position from intensity (1-16)
    // Intensity 16 is top (Y=0), Intensity 1 is bottom (Y=height)
    const y = imgHeight - (((intensityValue - 1) / 15) * imgHeight); // Scale 0-15 to 0-height


    // Clamp pointer position to ensure it stays within image bounds
    const clampedX = Math.max(0, Math.min(x, imgWidth));
    const clampedY = Math.max(0, Math.min(y, imgHeight));

    // Set pointer CSS position
    warmnessPointer.style.left = `${clampedX}px`;
    warmnessPointer.style.top = `${clampedY}px`;

    // Calculate pointer's RGB color for display
    const warmnessProgress = clampedX / imgWidth;
    const topRowColor = lerpColor(COOL_WHITE_RGB, WARM_YELLOW_RGB, warmnessProgress);

    const brightnessProgress = clampedY / imgHeight;
    const pointerRgbColor = lerpColor(topRowColor, [0, 0, 0], brightnessProgress);

    warmnessPointer.style.backgroundColor = `rgb(${pointerRgbColor[0]}, ${pointerRgbColor[1]}, ${pointerRgbColor[2]})`;

    // Update hidden input values
    warmnessValueInput.value = warmnessValue;
    intensityValueInput.value = intensityValue;
}

function startWarmnessIntensityDrag(e) {
    e.preventDefault();
    isDraggingWarmnessIntensity = true;
    const { warmness, intensity } = getWarmnessIntensityAndColorFromCoords(e);
    updateWarmnessIntensityPointer(warmness, intensity); // Updates position and color
    sendControlData();
}

function dragWarmnessIntensity(e) {
    if (!isDraggingWarmnessIntensity) return;
    const { warmness, intensity } = getWarmnessIntensityAndColorFromCoords(e);
    updateWarmnessIntensityPointer(warmness, intensity); // Updates position and color
    sendControlData();
}

function endWarmnessIntensityDrag() {
    isDraggingWarmnessIntensity = false;
}


// --- Event Listeners and Initial Setup ---
document.addEventListener('DOMContentLoaded', () => {
    // Initial display update when page loads
    reloadMainPage();
    updateLightModeDisplay();

    // Attach listeners for main mode selection
    getById('lightMode').addEventListener('change', () => {
        updateLightModeDisplay();
        sendControlData();
    });
    getById('fanSpeed').addEventListener('change', sendControlData);

    // Attach listener for add device button
    getById('add-device').addEventListener('click', openDeviceSelectionDialog);
    getById('search-devices-main').addEventListener('click', openDeviceSelectionDialog);
    searchForDevicesBtn.addEventListener('click', searchForDevices)
    getById('back-to-main-menu').addEventListener('click', goHome)
    // Attach listener for modal cancel button
    getById('cancel-device-selection').addEventListener('click', closeDeviceSelectionDialog);
    deviceSelectionOverlay.addEventListener('click', (e) => {
        if (e.target === deviceSelectionOverlay) {
            closeDeviceSelectionDialog();
        }
    });
});