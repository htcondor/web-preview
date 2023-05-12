// Powers the HTCSS User Map

const defaultIconScale = 48

function getIcon(iconScale){
    let iconSize = [iconScale*2.8, iconScale]
    let iconConfig = {
        iconUrl: "/web-preview/preview-user-map/assets/images/HTCondor_Bird.svg",
        iconSize: iconSize,
        iconAnchor: [iconSize[0]*.5, iconSize[1]],
        shadowUrl: "/web-preview/preview-user-map/assets/images/HTCondor_Bird_Shadow.svg",
        shadowAnchor: [iconSize[1], iconSize[1]*.6],
        shadowSize: iconSize
    }
    return L.icon(iconConfig)
}

function getScale(zoom){
    return defaultIconScale - Math.max(((defaultIconScale / 2) - zoom), 0)
}

function create_marker(location, iconScale){
    return [...Array(9).keys()]
        .map(x => x-5)
        .map(x => L.marker([location[0], location[1] + (x*360)], {icon: getIcon(iconScale)}))
}

async function get_icon_locations(){
    let response = await fetch("/web-preview/preview-user-map/assets/data/htcss-users.json")
    return response.json()
}

async function build_map(){

    const zoom = 3

    var map = L.map('map').setView([35, -90], zoom);

    L.tileLayer('https://api.mapbox.com/styles/v1/{id}/tiles/{z}/{x}/{y}?access_token={accessToken}', {
        attribution: 'Map data &copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors, Imagery © <a href="https://www.mapbox.com/">Mapbox</a>',
        maxZoom: 18,
        id: 'mapbox/dark-v11',
        tileSize: 512,
        zoomOffset: -1,
        accessToken: 'pk.eyJ1IjoidGFraW5nZHJha2UiLCJhIjoiY2wya3IyZGNvMDFyOTNsbnhyZjBteHRycSJ9.g6tRaqN8_iJxHgAQKNP6Tw'
    }).addTo(map);

    let iconLocations = await get_icon_locations()

    let markers = L.layerGroup(iconLocations.flatMap(x => create_marker(x, getScale(zoom))))

    markers.addTo(map)

    map.on("zoomend", () => {
        let currentZoom = map.getZoom();
        markers.eachLayer(x => x.setIcon(getIcon(getScale(currentZoom))))
    })
}

build_map()
