import React from "react";
import classNames from "classnames";
import { SearchIcon, XIcon } from "@heroicons/react/outline";

class ChatContainer extends React.Component {
	constructor(props) {
		super(props);
		this.state = {
			searchResult: [],
			searchFired: false
		};
	}

	doSearch(e) {
		let i;
		let q = e.target.value.trim().toLowerCase();
		let ct = this.props.container;
		let activeChatId = ct.state.activeChatId;

		if (q.length < 3) {
			this.setState({ searchFired: false, searchResult: [] });
			return;
		}

		if (!(activeChatId in ct.state.chatBoxDataCache)) {
			this.setState({ searchResult: [] });
			return;
		}

		let res = [];
		let msgList = ct.state.chatBoxDataCache[activeChatId];
		for (i = 0; i < msgList.length; i++) {
			let m = msgList[i];
			if (m.text.toLowerCase().includes(q))
				res.push(msgList[i]);
		}
		this.setState({ searchFired: true, searchResult: res });
	}

	render() {
		const offcanvas = this.props.offcanvas;
		const setOffcanvas = this.props.setOffcanvas;

		let searchResult;
		let sr = this.state.searchResult;

		if (sr.length == 0) {
			searchResult = this.state.searchFired ? (<div><p>Not Found!</p></div>) : "";
		} else {
			searchResult = sr.map(function ({ first_name, last_name, text }) {
				return (
					<div className="ml-4 space-y-2 bg-gray-100  px-4 py-2 max-w-md rounded-xl whitespace-pre-line line-clamp-1">
						<p className="font-semibold">{first_name + (last_name ? " " + last_name : "")}</p>
						<p className="text-lg" style={{ wordWrap: "break-word" }}>
						{text}
						</p>
					</div>
				);
			});
		}

		return (
			<div className={classNames("w-4/12 bg-white", offcanvas ? "block" : "hidden")}>
				<div className="flex items-center bg-white w-full pl-5 h-[80px] shadow-md sticky top-0">
					<h1 className="text-[20px] text-gray-500">Search Message</h1>
					<XIcon className="w-8 h-8 ml-auto mr-4 text-gray-400 cursor-pointer" onClick={() => setOffcanvas(false)} />
				</div>
				<div className="relative flex-grow my-4 rounded-lg mx-4">
					<div className="absolute inset-y-0 flex items-center pl-3 pointer-events-none">
						<SearchIcon className="h-5 w-5 text-gray-500" />
					</div>
					<input
						className="bg-gray-200 w-full h-10 pl-10 rounded-lg outline-none"
						type="text"
						placeholder="Search message"
						onChange={(e) => this.doSearch(e)}
					/>
				</div>
				<div className="space-y-6 flex flex-col  overflow-y-auto h-[77%] whitespace-nowrap p-4">
					{searchResult}
				</div>
			</div>
		);
	}
}

export default ChatContainer;
