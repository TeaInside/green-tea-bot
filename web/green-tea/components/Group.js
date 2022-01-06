import React from "react";

class Group extends React.Component {
    constructor(props) {
        super(props);
    }

    render() {
        let active = (this.props.container.state.activeChatId == this.props.groupId);
        let rclass = "flex items-center cursor-pointer p-2 ";
        if (active) {
            rclass += "bg-gray-200";
        } else {
            rclass += "hover:bg-gray-200";
        }
        return (
            <div onClick={(e) => this.props.container.fetchChatBoxData(this.props.groupId, this.props.groupName)} className={rclass}>
                <img className="w-12 h-12 rounded-full" src="profile.jpeg" alt="" />
                <div className="ml-4">
                    <h2>{"(" + this.props.groupId + ") " + "" + this.props.groupName}</h2>
                    <p className="text-gray-400">{this.props.container.getLastMessage(this.props.groupId)}</p>
                </div>
            </div>
        );
    }
}

export default Group;
