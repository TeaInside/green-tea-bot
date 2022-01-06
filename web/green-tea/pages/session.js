import React from "react";
import Login from "../components/Login";

class Session extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            sessToken: null,
        };
    }

    componentDidMount() {
        let token = localStorage.getItem("token");
        this.setState({ sessToken: token });
        if (!token && window.location.pathname != "/") {
            window.location = "/";
        }
    }

    render() {
        if (!this.state.sessToken) {
            return <Login />;
        }

        return this.props.page;
    }
}

export default Session;
