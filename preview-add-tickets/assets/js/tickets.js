let idx;
window.onload = main;

function main(){
    get_url_ticket()
    set_up_search_bar()

    let ticket_search = document.getElementById("ticket-search")
    ticket_search.addEventListener("keyup", populate_search)
}

function get_url_ticket(){
    let params = new URLSearchParams(location.search);
    if(params.has("ticket")){
        get_ticket_and_populate(params.get('ticket'))
    }
}

async function set_up_search_bar(){
    let idx_data = await fetch("/assets/data/search_indexes/ticket_index.json")
    let idx_json = await idx_data.json()

    idx = lunr.Index.load(idx_json)

    document.getElementById("search-loading").hidden = true
    document.getElementById("search-bar").hidden = false
}

function create_results_html(ticket){
    let html =  "<div id='search-card' class='card'>" +
                    "<div class='card-body'>" +
                        "<div class='card-title'>" +
                            "<a onclick='get_ticket_and_populate(" + ticket.id + ")'>" + ticket.title + "</a>" +
                        "</div>" +
                    "</div>" +
                "</div>"

    return html
}

async function populate_search() {

    // Remove the current results
    search_results = document.getElementById("search-results")
    search_results.innerHTML = ""

    ticket_search = document.getElementById("ticket-search")

    let query = ticket_search.value

    if(query == ""){
        return
    }

    results = idx.search(query)

    for (const result of results) {
        let new_result_node = document.createElement("div")
        search_results.appendChild(new_result_node)

        let ticket = await get_ticket(result.ref)
        new_result_node.innerHTML = create_results_html(ticket)
    }
}

// This code is to populate the ticket elements
async function get_ticket(id){

    let file_path = "/assets/data/tickets/" + id + ".json"

    results = await fetch(file_path)
    json = await results.json()
    return json
}

async function get_ticket_and_populate(id){
    let ticket = await get_ticket(id)
    populate_page(ticket)
}

var populate_page = (ticket) => {
    document.getElementById("ticket").hidden = true

    // Title
    document.getElementById("title").innerText = ticket.title

    // Description
    document.getElementById("description").innerHTML = ticket.description

    // Remarks
    document.getElementById("remarks").innerHTML = ticket.remarks

    // Properties
    document.getElementById("type").innerText = ticket.type
    document.getElementById("status").innerText = ticket.status
    document.getElementById("fixed-version").innerText = ticket.fixed_version
    document.getElementById("priority").innerText = ticket.priority
    document.getElementById("assigned-to").innerText = ticket.assigned_to
    document.getElementById("creator").innerText = ticket.creator
    document.getElementById("customer-group").innerText = ticket.customer_group
    document.getElementById("notify").innerText = ticket.notify
    document.getElementById("last-change").innerText = ticket.last_change
    document.getElementById("created").innerText = ticket.created
    document.getElementById("broken-version").innerText = ticket.broken_version
    document.getElementById("subsystem").innerText = ticket.subsystem
    document.getElementById("derived-from").innerText = ticket.derived_from
    document.getElementById("visibility").innerText = ticket.visibility
    document.getElementById("due-date").innerText = ticket.due_date

    // Derived Tickets
    if(ticket.derived_tickets != ""){
        document.getElementById("derived-tickets").hidden = false
        document.getElementById("derived-tickets-content").innerHTML = ticket.derived_tickets
    } else {
        document.getElementById("derived-tickets").hidden = true
    }

    // Check_ins
    if(ticket.check_ins != ""){
        document.getElementById("check-ins").hidden = false
        document.getElementById("check-ins-content").innerHTML = ticket.check_ins
    } else {
        document.getElementById("check-ins").hidden = true
    }

    // Attachments
    if(ticket.attachments != ""){
        document.getElementById("attachments").hidden = false
        document.getElementById("attachments-content").innerHTML = ticket.attachments
    } else {
        document.getElementById("attachments").hidden = true
    }

    document.getElementById("ticket").hidden = false
}