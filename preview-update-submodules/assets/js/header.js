function HeaderToggles(toggles){
    this.toggles = toggles

    this.toggle = function(toggle){
        // If this drawer is open then close it
        if(toggle.is_open){
            toggle.close()

        // Otherwise Open this drawer and close the other ones
        } else {
            for(const general_toggle of toggles){
                if( general_toggle.id == toggle.id ){
                    general_toggle.open()
                } else {
                    general_toggle.close()
                }
            }
        }
    }

    this.close = function(){
        for(const toggle of toggles){
            toggle.close()
        }
    }

    for(const toggle of toggles){
        toggle.toggle_node.addEventListener("click", () => this.toggle(toggle))
    }

    const drawer_container = document.getElementById("header-drawer")
    document.addEventListener('click', ( event ) => {
        if (
            !event.target.classList.contains("drawer-toggle") &&
            drawer_container !== event.target &&
            !drawer_container.contains(event.target)
        ) {
            this.close()
        }
    });
}

function HeaderToggle(id){
    this.id = id
    this.is_open = false
    this.toggle_node = document.getElementById(id + "-toggle")
    this.drawer_container = document.getElementById("header-drawer")
    this.drawer_node = document.getElementById(id + "-drawer")

    document.addEventListener("scroll", () => this.close())

    this.toggle = function(){
        if(this.is_open){
            this.close()
        } else {
            this.open()
        }
    }
    this.close = function(){
        if(!this.is_open){return}

        this.drawer_node.hidden = true
        this.drawer_container.hidden = true
        this.toggle_node.classList.remove("active")

        this.is_open = false
    }
    this.open = function(){
        if(this.is_open){return}

        this.drawer_node.hidden = false
        this.drawer_container.hidden = false
        this.toggle_node.classList.add("active")

        this.is_open = true
    }
}

const software = new HeaderToggle("software-desktop")
const documentation = new HeaderToggle("help-and-support-desktop")
const commmunity = new HeaderToggle("community-desktop")
const about = new HeaderToggle("about-desktop")
const header_toggles = new HeaderToggles([software, documentation, commmunity, about])