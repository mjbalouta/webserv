//to change the 'no file selected' into the file name uploaded
const fileInput = document.getElementById('file-upload');
const fileNameDisplay = document.getElementById('file-name');

fileInput.addEventListener('change', function () {
	const fileName = this.files[0] ? this.files[0].name : "No file selected.";
	fileNameDisplay.textContent = fileName;
});

//to handle the hiding of the different divs and only show one
document.querySelectorAll('a[href^="#"]').forEach(link => {
	link.addEventListener('click', function (e) {
		//to get the id from the href
		const targetId = this.getAttribute('href');
		//only run logic if it is an internal link starting with #
		if (targetId.startsWith('#')) {
			e.preventDefault();

			 if (targetId === '#gallery')
                refreshGallery();

			const targetSection = document.querySelector(targetId);
			const mainContainer = document.getElementById('main-container');
			//start fade out
			mainContainer.style.opacity = "0";
			mainContainer.style.transform = "translateY(4px) scale(0.98)";
			setTimeout(() => {
				//hide all the sections
				document.querySelectorAll('.content-section').forEach(section => {
				section.classList.add('hidden'); //because we're using tailwind, otherwise flex wouldn't work properly
			})

			//show only the clicked one
			if (targetSection)
			{
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

				//to update the url hash without reloading (allows using back button)
				window.history.pushState(null, null, targetId);

				requestAnimationFrame(() => {
					setTimeout(() => {
						mainContainer.style.opacity = "1";
						mainContainer.style.transform = "translateY(0) scale(1)";
					}, 50);
				});
			}
		}, 300);
	}
});
});

//to prevent the button 'Search Info' from relocating to index.html
//check if the URL has a hash or query params
window.addEventListener('load', () => {
	const hash = window.location.hash;
	const urlParams = new URLSearchParams(window.location.search);

	// if there is a search query (?path=...), force the view to Method Testing
	if (urlParams.has('path')) 
	{
		const testingLink = document.querySelector('a[href="#method-testing"]');
		if (testingLink) 
			testingLink.click();
	} 
	else if (hash) 
	{
		// if there's just a hash (#gallery), go there
		const link = document.querySelector(`a[href="${hash}"]`);
		if (link) link.click();
	}
});

//to handle the button 'upload file'
const uploadForm = document.getElementById('post-upload-form');

if (uploadForm) {
    uploadForm.addEventListener('submit', function (e) {
        e.preventDefault(); // This is the magic part: no redirect to /upload

        const formData = new FormData(this);
        const requestDetailsContainer = document.querySelector('#method-testing ul#request-details');

        // Optional: Visual feedback that the request is firing
        requestDetailsContainer.style.opacity = "0.5";

        fetch('/upload', {
            method: 'POST',
            body: formData
        })
        .then(response => response.text()) // Get the full HTML page back from C++
        .then(html => {
            // Create a temporary DOM to parse the incoming HTML
            const parser = new DOMParser();
            const doc = parser.parseFromString(html, 'text/html');

            // 1. Find the new content that C++ replaced in the 'POST_REQUEST_DETAILS' block
            const newRequestContent = doc.querySelector('#method-testing ul#request-details').innerHTML;

            // 2. Inject it into our current page without reloading
            requestDetailsContainer.innerHTML = newRequestContent;
            requestDetailsContainer.style.opacity = "1";

            // 3. Refresh the gallery list in the background
            if (typeof refreshGallery === "function") {
                refreshGallery();
            }

            // 4. Reset the "No file selected" text
            uploadForm.reset();
            document.getElementById('file-name').textContent = "No file selected";
        })
        .catch(err => {
            console.error("Upload failed:", err);
            requestDetailsContainer.innerHTML = "<li>Error: Server connection lost.</li>";
            requestDetailsContainer.style.opacity = "1";
        });
    });
}

/* HANDLE GALLERY */

const CURRENT_UPLOAD_PATH = "{{UPLOAD_PATH}}";
	
// Function to refresh the gallery content
async function refreshGallery() {
    try {
        const response = await fetch(CURRENT_UPLOAD_PATH); // Fetch the page
        const text = await response.text();
        
        // Parse the text to find the gallery content
        const parser = new DOMParser();
        const doc = parser.parseFromString(text, 'text/html');
        const newGalleryContent = doc.querySelector('#gallery ul').innerHTML;
        
        document.querySelector('#gallery ul').innerHTML = newGalleryContent;
    } catch (err) {
        console.error("Failed to refresh gallery:", err);
    }
}

/**
 * Event Listener for Delete Buttons
 * Handles the DELETE request and updates the gallery UI without a page reload.
 */
document.addEventListener('click', function(e) {
    // Check if the clicked element (or its parent) has the delete-btn class
    const deleteBtn = e.target.closest('.delete-btn');
    
    if (deleteBtn) {
        // Retrieve the filename from the data attribute
        const fileName = deleteBtn.getAttribute('data-filename');
        
        // Safety confirmation before performing the deletion
        if (confirm(`Are you sure you want to delete "${fileName}"?`)) {
            
            // Send the DELETE request to the server
            fetch(`/delete?file=${encodeURIComponent(fileName)}`, {
                method: 'DELETE'
            })
            .then(response => {
                if (response.ok) {
                    console.log(`Successfully deleted: ${fileName}`);
                    // Trigger the UI refresh to show the updated file list
                    if (typeof refreshGallery === "function") {
                        refreshGallery();
                    }
                } else {
                    // Handle server-side errors (e.g., file not found or permission denied)
                    alert(`Failed to delete file: ${fileName}. Server responded with status ${response.status}`);
                }
            })
            .catch(error => {
                // Handle network errors
                console.error("Error during delete fetch:", error);
                alert("A network error occurred while trying to delete the file.");
            });
        }
    }
});

