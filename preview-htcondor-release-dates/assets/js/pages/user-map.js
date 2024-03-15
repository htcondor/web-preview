// Powers the HTCSS User Map

const defaultIconScale = 42

function getIcon(iconScale){
    let iconSize = [iconScale*2.8, iconScale]
    let iconConfig = {
        iconUrl: "/web-preview/preview-htcondor-release-dates/assets/images/icons/star_red.svg",
        iconSize: iconSize,
        iconAnchor: [iconSize[0]*.5, iconSize[1]],
        shadowUrl: "/web-preview/preview-htcondor-release-dates/assets/images/icons/small_shadow.svg",
        shadowAnchor: [iconSize[1]/2, (iconSize[1]*.6)/2],
        shadowSize: [iconSize[0]*.55, iconSize[1]*.3]
    }
    return L.icon(iconConfig)
}

function getScale(zoom){
    return defaultIconScale * (.5 + (.03*zoom))
}

function create_marker(location, iconScale, title){
    // Define jitter in degrees, 10 miles is approximately 1/69 degrees of latitude
    const jitterRangeLat = 10 / 69;
    // Longitude varies, use the cosine of the latitude (in radians)
    const jitterRangeLng = jitterRangeLat / Math.cos(location[0] * Math.PI / 180);

    // Create an array to hold the markers
    let markers = [];

    // Adjust this value if you want more or fewer markers
    const numberOfMarkers = 1; // For example, if you only want one marker per location

    for (let i = 0; i < numberOfMarkers; i++) {
        // Generate a random jitter within the range for latitude and longitude
        const jitterLat = (Math.random() - 0.5) * jitterRangeLat;
        const jitterLng = (Math.random() - 0.5) * jitterRangeLng;

        // Apply jitter and ensure the latitude and longitude are within bounds
        const newLat = Math.min(Math.max(location[0] + jitterLat, -90), 90);
        const newLng = ((location[1] + jitterLng + 180) % 360) - 180;

        // Create a marker with jitter applied and add it to the array
        const marker = L.marker([newLat, newLng], {icon: getIcon(iconScale)});
        if(title) {
            marker.bindPopup(title);
        }
        markers.push(marker);
    }

    // Return the array of markers
    return markers;
}


async function get_spreadsheet_values(){
    let res = await fetch("https://chtc.github.io/data-cache/data/htcss_user_registry.csv")

    let text = await res.text()

    let data = Papa.parse(text, {header:true})

    let geocodes = data['data'].map(x => {
        return {
            longitude: parseFloat(x['Longitude']),
            latitude: parseFloat(x['Latitude']),
            ...x
        }
    })

    let cleanGeocodes = geocodes.reduce((p, c) => {
        if(isNaN(c['latitude']) || isNaN(c['longitude'])) {
            return p
        }
        p.push(c)
        return p
    }, [])

    if(cleanGeocodes.length === 0) {
        console.error("No clean values were returned from the spreadsheet")
    }

    return cleanGeocodes
}

async function get_manual_values() {
    let response = await fetch("/web-preview/preview-htcondor-release-dates/assets/data/htcss-users.json")

    let data = await response.json()

    let geocodes = data.map(x => {
        return {
            longitude: parseFloat(x[0]),
            latitude: parseFloat(x[1])
        }
    })

    return geocodes
}

class UserMap {
    constructor() {
        let urlParams = new URLSearchParams(window.location.search)
        this.zoom = urlParams.get("zoom") ? urlParams.get("zoom") : 3
        this.viewerLatitude = urlParams.get("latitude") ? urlParams.get("latitude") : 35
        this.viewerLongitude = urlParams.get("longitude") ? urlParams.get("longitude") : -90
        this.scrollWheelZoom = urlParams.get("scrollWheelZoom") ? urlParams.get("scrollWheelZoom") !== '0' : true

        var map = L.map('map', {scrollWheelZoom: this.scrollWheelZoom}).setView([this.viewerLatitude, this.viewerLongitude], this.zoom);

        L.tileLayer('https://api.mapbox.com/styles/v1/{id}/tiles/{z}/{x}/{y}?access_token={accessToken}', {
            attribution: 'Map data &copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors, Imagery © <a href="https://www.mapbox.com/">Mapbox</a>',
            maxZoom: 17,
            id: 'mapbox/satellite-streets-v12',
            tileSize: 512,
            zoomOffset: -1,
            accessToken: 'pk.eyJ1IjoidGFraW5nZHJha2UiLCJhIjoiY2wya3IyZGNvMDFyOTNsbnhyZjBteHRycSJ9.g6tRaqN8_iJxHgAQKNP6Tw',
        }).addTo(map);

        this.markerLayer = L.layerGroup()
        this.markerLayer.addTo(map)

        map.on("zoomend", () => {
            this.zoom = map.getZoom();
            this.markerLayer.eachLayer(x => x.setIcon(getIcon(getScale(this.zoom))))
        })

        this.markerCount = 0
    }

    updateMarkerCount(numOfNewMarkers) {
        this.markerCount += numOfNewMarkers
        document.getElementById("org-count").textContent = this.markerCount
    }

    addIcon(coordinates, title) {
        const markers = create_marker(coordinates, getScale(this.zoom), title)
        markers.forEach(x => this.markerLayer.addLayer(x))
    }


    async addIcons(getter) {
        let iconLocations = await getter()
        iconLocations.forEach(x => this.addIcon([x['longitude'], x['latitude']], x['Organization Name ( Optional: Add if you want displayed ) ']))
        this.updateMarkerCount(iconLocations.length)
    }
}

function main(){
    const map = new UserMap();
    map.addIcons(get_spreadsheet_values)
    map.addIcons(get_manual_values)
}

main()