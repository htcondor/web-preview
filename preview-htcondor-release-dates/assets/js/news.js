document.addEventListener('DOMContentLoaded', function() {
    // The button to toggle between different news categories
    const queryButtons = [
        document.getElementById('news-toggle'),
        document.getElementById('featured-users-toggle'),
        document.getElementById('tech-blog-toggle'),
    ];

    // The card groups for each category
    const cardGroups = [
        document.querySelectorAll('.news-card-news'),
        document.querySelectorAll('.news-card-feature'),
        document.querySelectorAll('.news-card-tech-blog'),
    ];

    // The 'All' button to show all articles
    const allCardsButton = document.getElementById('all-news-toggle');

    function enableQueryButton(button) {
        if (button == queryButtons[0]) {
            updateURL('news');
        } else if (button == queryButtons[1]) {
            updateURL('featured-users');
        } else if (button == queryButtons[2]) {
            updateURL('tech-blog');
        }

        // Remove active and primary classes from all buttons
        queryButtons.forEach(btn => {
            btn.classList.remove('active', 'btn-primary');
            btn.classList.add('btn-outline-primary');
        });
        allCardsButton.classList.remove('active', 'btn-primary');

        // Add active and primary classes to the clicked button
        button.classList.remove('btn-outline-primary');
        button.classList.add('active', 'btn-primary');
    }

    function updateURL(category) {
        const url = new URL(window.location);
        if (category) {
            url.searchParams.set('category', category);
        } else {
            url.searchParams.delete('category');
        }
        history.pushState(null, '', url);
    }

    // Setup the 'all news' button
    allCardsButton.addEventListener('click', function() {
        // Enable the clicked button and disable others
        enableQueryButton(this);

        // Update the URL to remove the category parameter
        updateURL(null);

        // Show all cards
        cardGroups.forEach(cardGroup => {
            cardGroup.forEach(card => {
                card.parentElement.style.display = 'block';
            });
        });
    });

    // Setup the toggle buttons
    queryButtons.forEach(button => {
        button.addEventListener('click', function() {
            // Enable the clicked button and disable others
            enableQueryButton(this);
            
            // Hide all cards first
            cardGroups.forEach(cardGroup => {
                cardGroup.forEach(card => {
                    card.parentElement.style.display = 'none';
                });
            });

            // Show the cards for the clicked button
            const index = queryButtons.indexOf(this);
            if (index >= 0 && index < cardGroups.length) {
                cardGroups[index].forEach(card => {
                    card.parentElement.style.display = 'block';
                });
            }
        });
    });

    // Handle initial filtering from URL
    const params = new URLSearchParams(window.location.search);
    const category = params.get('category');

    switch (category) {
        case 'news':
            queryButtons[0].click();
            break;
        case 'featured-users':
            queryButtons[1].click();
            break;
        case 'tech-blog':
            queryButtons[2].click();
            break;
    }
});