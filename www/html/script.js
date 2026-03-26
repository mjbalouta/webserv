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

// Upload handler (if you still want async upload)
document.getElementById('upload-form').addEventListener('submit', async function (e) {
    e.preventDefault();

    if (!fileInput.files || !fileInput.files[0]) {
        alert('Please choose a file first.');
        return;
    }

    const formData = new FormData();
    formData.append('file', fileInput.files[0]);

    try {
        const response = await fetch('/upload', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            fileInput.value = '';
            fileNameDisplay.textContent = 'No file selected';
            await refreshGallery();
        } else {
            console.error('Upload failed:', response.status, response.statusText);
        }
    } catch (error) {
        console.error('Upload error:', error);
    }
});

window.addEventListener('DOMContentLoaded', () => {
    refreshGallery();
});

async function refreshGallery() {
    try {
        // Keep this as /index.html so #gallery ul exists in parsed HTML
        const response = await fetch('/index.html', { cache: 'no-store' });
        const text = await response.text();

        const parser = new DOMParser();
        const doc = parser.parseFromString(text, 'text/html');
        const srcList = doc.querySelector('#gallery ul');
        const dstList = document.querySelector('#gallery ul');

        if (!srcList || !dstList) {
            console.error('Gallery list not found in source or destination');
            return;
        }
        dstList.innerHTML = srcList.innerHTML;
    } catch (err) {
        console.error('Failed to refresh gallery:', err);
    }
}

