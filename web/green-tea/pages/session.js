import React from "react";
import Login from "../components/Login";

class Session extends React.Component {
	constructor(props) {
		super(props);
		this.state = {
			sess_token: null
		};
	}
	
	render() {

		if (this.state.sess_token == null)
			return (<Login/>);
		return (this.props.page);
	}
};

export default Session;
