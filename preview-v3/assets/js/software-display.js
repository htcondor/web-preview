let SoftwareDisplay = function(){
    this.software_headers = document.getElementsByClassName("desktop-header")
    this.software_briefs = document.getElementsByClassName("desktop-software-brief")

    for (const software_brief of this.software_briefs) {
        if( document.getElementById(software_brief.id).classList.contains("open") ){
            this.open_id = software_brief.id.replace("-desktop-brief", "")
        }
    }

    this.close_brief = function(id){
        let brief_id = id + "-desktop-brief";
        let button_id = id + "-desktop-button";
        document.getElementById(button_id).classList.add("collapsed")
        document.getElementById(brief_id).classList.remove("open")
    }

    this.open_brief = function(id){
        let brief_id = id + "-desktop-brief";
        let button_id = id + "-desktop-button";
        document.getElementById(button_id).classList.remove("collapsed")
        document.getElementById(brief_id).classList.add("open")
    }

    this.toggle_brief = async function(event){
        let toggled_id = event.currentTarget.id.replace("-desktop-header", "")

        // If the toggled option is already open we only have to close a brief
        if( this.open_id != toggled_id ) {

            this.close_brief(this.open_id);

            // If a brief was open then wait until it is closed to open one
            if( this.open_id ){
                setTimeout(() => this.open_brief(toggled_id), 300);

            } else {
                this.open_brief(toggled_id)
            }
            this.open_id = toggled_id

        }
    }

    for (const software_header of this.software_headers) {
        software_header.addEventListener("click", (event) => this.toggle_brief(event))
    }
}

const software_display = new SoftwareDisplay()