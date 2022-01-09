import React from "react";

class Group extends React.Component {
    constructor(props) {
        super(props);
    }

    render() {
        let active = this.props.container.state.activeChatId == this.props.groupId;
        let rclass = "flex items-center cursor-pointer p-2 ";
        if (active) {
            rclass += "bg-gray-200";
        } else {
            rclass += "hover:bg-gray-200";
        }
        let thisGroup = this;
        return (
            <div
                onClick={async function(e) {
                    await thisGroup.props.container.fetchChatBoxData(
                        thisGroup.props.groupId,
                        thisGroup.props.groupName);

                    let x = document.getElementById("chatBoxCt");
                    x.scrollTo(0, x.scrollHeight);
                }}
                className={rclass}>
                <img className="w-12 h-12 rounded-full" src="profile.jpeg" alt="" />
                <div className="ml-4 w-9/12 overflow-hidden whitespace-nowrap">
                    <h2>{"(" + this.props.groupId + ") " + "" + this.props.groupName}</h2>
                    <p className="text-gray-400 truncate">{this.props.container.getLastMessage(this.props.groupId)}</p>
                </div>
            </div>
        );
    }
}

export default Group;
