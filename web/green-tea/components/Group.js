import React from "react";

class Group extends React.Component {
    constructor(props) {
        super(props);
    }

    render() {
        return (
            <div onClick={(e) => this.props.container.fetchChatBoxData(this.props.groupId, this.props.groupName)} className="flex items-center cursor-pointer p-2 hover:bg-gray-200">
                <img className="w-12 h-12 rounded-full" src="profile.jpeg" alt="" />
                <div className="ml-4">
                    <h2>{"(" + this.props.groupId + ") " + "" + this.props.groupName}</h2>
                    <p className="text-gray-400">{this.props.lastMessage}</p>
                </div>
            </div>
        );
    }
}

export default Group;
