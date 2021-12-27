import Head from "next/head";
import ChatBox from "../components/ChatBox";
import GroupList from "../components/GroupList";
import Sidebar from "../components/Sidebar";

function chatlog({ data }) {
    return (
        <div className="flex">
            <Head>
                <title>GreenTea Dashboard</title>
                <link rel="icon" href="/greentea.ico" />
            </Head>
            <Sidebar />
            <GroupList list={data.msg.data} />
            <ChatBox />
        </div>
    );
}

export default chatlog;

export async function getServerSideProps(context) {
    // const products = await fetch("https://fakestoreapi.com/products").then((res) => res.json());
    const res = await fetch("https://www.teainside.org/fp123/get_group_list.json");
    const data = await res.json();

    return {
        props: {
            data,
        },
    };
}
