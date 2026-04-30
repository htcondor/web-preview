// Generate idx using command at root:
// echo "$(cat assets/data/search_documents/ticket_search.json)" | node assets/js/build_ticket_index.js > assets/data/search_indexes/ticket_index.json
//
// ticket_search.json contains the json of just space delimited text that occurs in the ticket organized by category

var lunr = require('lunr'),
    stdin = process.stdin,
    stdout = process.stdout,
    buffer = []

stdin.resume()
stdin.setEncoding('utf8')

stdin.on('data', function (data) {
    buffer.push(data)
})

stdin.on('end', function () {
    var documents = JSON.parse(buffer.join(''))

    var idx = lunr(function () {
        this.ref("id")
        this.field("id")
        this.field("title")
        this.field("remarks")
        this.field("type")
        this.field("status")
        this.field("fixed_version")
        this.field("priority")
        this.field("assigned_to")
        this.field("creator")
        this.field("customer_group")
        this.field("notify")
        this.field("last_change")
        this.field("created")
        this.field("broken_version")
        this.field("subsystem")
        this.field("derived_from")
        this.field("rust")
        this.field("visibility")
        this.field("due_date")
        this.field("description")
        this.field("derived_tickets")
        this.field("check_ins")
        this.field("attachments")

        documents.forEach(function (doc) {
            this.add(doc)
        }, this)
    })

    stdout.write(JSON.stringify(idx))
})
