import React from "react";
import Login from "../components/Login";

class Session extends React.Component {
	constructor(props) {
		super(props);
		this.state = {
			sessToken: null
		};
	}

	componentDidMount() {
		this.setState({sessToken: localStorage.getItem("token")});
	}
	
	render() {
		if (!this.state.sessToken)
			return (<Login/>);

		return (this.props.page);
	}
};

export default Session;
