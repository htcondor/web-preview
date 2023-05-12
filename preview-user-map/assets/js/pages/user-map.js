// Powers the HTCSS User Map

const defaultIconScale = 36

function getIcon(iconScale){
    let iconSize = [iconScale*2.8, iconScale]
    let iconConfig = {
        iconUrl: "assets/images/HTCondor_Bird.svg",
        iconSize: iconSize,
        iconAnchor: [iconSize[0]*.5, iconSize[1]],
        shadowUrl: "/assets/images/HTCondor_Bird_Shadow.svg",
        shadowAnchor: [iconSize[1], iconSize[1]*.6],
        shadowSize: iconSize
    }
    return L.icon(iconConfig)
}

function create_marker(location, iconScale){
    return L.marker(location, {icon: getIcon(iconScale)})
}

async function get_icon_locations(){
    let response = await fetch("/assets/data/htcss-users.json")
    return response.json()
}

async function build_map(){
    var map = L.map('map').setView([51.505, -90], 4);

    L.tileLayer('https://api.mapbox.com/styles/v1/{id}/tiles/{z}/{x}/{y}?access_token={accessToken}', {
        attribution: 'Map data &copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors, Imagery © <a href="https://www.mapbox.com/">Mapbox</a>',
        maxZoom: 18,
        id: 'mapbox/dark-v11',
        tileSize: 512,
        zoomOffset: -1,
        accessToken: 'pk.eyJ1IjoidGFraW5nZHJha2UiLCJhIjoiY2wya3IyZGNvMDFyOTNsbnhyZjBteHRycSJ9.g6tRaqN8_iJxHgAQKNP6Tw'
    }).addTo(map);

    let iconLocations = await get_icon_locations()

    let markers = L.layerGroup(iconLocations.map(x => create_marker(x, defaultIconScale)))

    markers.addTo(map)

    map.on("zoomend", () => {
        let currentZoom = map.getZoom();

        console.log("Current Zoom", currentZoom)

        let scale = defaultIconScale - Math.max(((defaultIconScale / 2) - currentZoom), 0)

        console.log(scale)

        markers.eachLayer(x => x.setIcon(getIcon(scale)))
    })
}

build_map()
