//to change the 'no file selected' into the file name uploaded
const fileInput = document.getElementById('file-upload');
const fileNameDisplay = document.getElementById('file-name');

fileInput.addEventListener('change', function () {
    const fileName = this.files[0] ? this.files[0].name : "No file selected.";
    fileNameDisplay.textContent = fileName;
});

// Keep this as the single section-switching flow
document.querySelectorAll('a[href^="#"]').forEach(link => {
    link.addEventListener('click', function (e) {
        const targetId = this.getAttribute('href');
        if (!targetId || !targetId.startsWith('#'))
            return;

        e.preventDefault();

        // Fix: targetId includes '#'
        if (targetId === '#gallery')
            refreshGallery();

        const targetSection = document.querySelector(targetId);
        const mainContainer = document.getElementById('main-container');

        mainContainer.style.opacity = "0";
        mainContainer.style.transform = "translateY(4px) scale(0.98)";

        setTimeout(() => {
            document.querySelectorAll('.content-section').forEach(section => {
                section.classList.add('hidden');
            });

            // Your required block kept as-is
            if (targetSection) {
                targetSection.classList.remove('hidden');

                mainContainer.classList.remove(
                    'max-w-lg', 'max-w-3xl', 'max-w-6xl',
                    'backdrop-blur-md', 'rounded-2xl', 'p-8',
                    'shadow-[0_0_15px_rgba(100,20,120,0.8)]',
                    'bg-black-900/15'
                );

                if (targetId == '#method-testing')
                    mainContainer.classList.add('max-w-6xl');
                else {
                    mainContainer.classList.add(
                        'backdrop-blur-md', 'rounded-2xl', 'p-8',
                        'shadow-[0_0_15px_rgba(100,20,120,0.8)]',
                        'bg-black-900/15'
                    );
                }
                if (targetId == "#server-info")
                    mainContainer.classList.add('max-w-lg');
                else
                    mainContainer.classList.add('max-w-3xl');

                window.history.pushState(null, null, targetId);

                requestAnimationFrame(() => {
                    setTimeout(() => {
                        mainContainer.style.opacity = "1";
                        mainContainer.style.transform = "translateY(0) scale(1)";
                    }, 50);
                });
            }
        }, 300);
    });
});

// Handle GET form submission without page reload (to stay in the same page when clicking 'search info')
document.getElementById('get-method-form').addEventListener('submit', async function (e) {
    e.preventDefault();

    const pathInput = document.getElementById('get-path-input');
    const path = pathInput.value;

    try {
        // Fetch with query parameter, don't navigate
        const response = await fetch(`/index.html?path=${encodeURIComponent(path)}`, {
            cache: 'no-store'
        });
        const text = await response.text();

        const parser = new DOMParser();
        const doc = parser.parseFromString(text, 'text/html');

        // Update the GET request details and location details from the response
        const srcRequestDetails = doc.querySelector('#request-details');
        const srcLocationDetails = doc.querySelector('#location-details');
        const dstRequestDetails = document.querySelector('#request-details');
        const dstLocationDetails = document.querySelector('#location-details');

        if (srcRequestDetails && dstRequestDetails) {
            dstRequestDetails.innerHTML = srcRequestDetails.innerHTML;
        }
        if (srcLocationDetails && dstLocationDetails) {
            dstLocationDetails.innerHTML = srcLocationDetails.innerHTML;
        }

        // Stay on #method-testing section
        const testingLink = document.querySelector('a[href="#method-testing"]');
        if (testingLink) {
            testingLink.click();
        }
    } catch (error) {
        console.error('Search error:', error);
    }
});

// Helper function to show upload status
function showUploadStatus(message, statusCode, isError = false) {
    const statusDiv = document.getElementById('upload-status');
    const statusText = document.getElementById('upload-status-text');
    
    statusText.textContent = `${message}`;
    statusDiv.classList.remove('hidden');
    
    if (isError) {
        statusDiv.classList.remove('text-green-900');
        statusDiv.classList.add('text-red-900');
    } else {
        statusDiv.classList.remove('text-red-900');
        statusDiv.classList.add('text-green-900');
    }
    
    // Auto-hide after 5 seconds if successful
    if (!isError) {
        setTimeout(() => {
            statusDiv.classList.add('hidden');
        }, 5000);
    }
}

// Handle file upload
document.getElementById('upload-form').addEventListener('submit', async function(e) {
    e.preventDefault();
    
    const fileInput = document.getElementById('file-upload');
    if (!fileInput.files || !fileInput.files[0]) {
        showUploadStatus('Please choose a file first.', 400, true);
        return;
    }

    const fileName = fileInput.files[0].name;
    const formData = new FormData();
    formData.append('file', fileInput.files[0]);
    
    try {
        const response = await fetch('/upload', {
            method: 'POST',
            body: formData
        });
        
        if (response.status === 201) {
            showUploadStatus(`✓ File "${fileName}" created successfully`, 201, false);
            
            fileInput.value = '';
            document.getElementById('file-name').textContent = 'No file selected';
            
            await refreshGallery();
            
            // Navigate to gallery after 1 second
            setTimeout(() => {
                const galleryLink = document.querySelector('a[href="#gallery"]');
                if (galleryLink) {
                    galleryLink.click();
                }
            }, 1000);
        } else if (response.status === 403) {
            showUploadStatus('Permission denied: Cannot write to upload directory', 403, true);
        } else if (response.status === 400) {
            showUploadStatus('Bad request: Invalid file format or parameters', 400, true);
        } else if (response.status === 500) {
            showUploadStatus('Server error: Failed to save file', 500, true);
        } else {
            showUploadStatus(`Upload failed with status ${response.status}`, response.status, true);
        }
    } catch (error) {
        console.error('Upload error:', error);
        showUploadStatus('Network error occurred during upload', 0, true);
    }
});

window.addEventListener('DOMContentLoaded', () => {
    refreshGallery();
});

// Function to refresh the gallery content
async function refreshGallery() {
    try {
        const response = await fetch('/index.html', { cache: 'no-store' });
        const text = await response.text();
        
        const parser = new DOMParser();
        const doc = parser.parseFromString(text, 'text/html');
        const newGalleryContent = doc.querySelector('#gallery ul');
        const currentGallery = document.querySelector('#gallery ul');
        
        if (newGalleryContent && currentGallery) {
            currentGallery.innerHTML = newGalleryContent.innerHTML;
        }
    } catch (err) {
        console.error("Failed to refresh gallery:", err);
    }
}


