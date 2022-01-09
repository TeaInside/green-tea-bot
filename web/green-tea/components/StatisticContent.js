import React from "react";
import GraphStatistic from "./GraphStatistic";
import TableStatistic from "./TableStatistic";

export default class StatisticContent extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            loading: false,
            data: [],
        };
    }

    async componentDidMount() {
        const res = await fetch("https://greentea-api.teainside.org/api.php?action=get_msg_count_group");
        const data = await res.json();

        this.setState({
            data: data.msg.data,
            loading: false,
        });
    }

    render() {
        let i = 0;
        const data = this.state.data;

        if (this.state.loading || data.length == 0)
            return (
                <div>
                    <h1>Loading ...</h1>
                </div>
            );

        return (
            <div className="w-full h-screen bg-cream overflow-y-scroll whitespace-nowrap py-10">
                <h1 className="text-center  text-[3em]">STATISTIC</h1>

                <div className="grid grid-cols-3 grid-rows-2 mx-16 mt-8 gap-x-10">
                    <GraphStatistic />
                    <TableStatistic />
                </div>
                <div className="mx-16 mt-8  grid grid-cols-2  h-[600px] ">
                    <div className="bg-indigo-800 overflow-y-scroll whitespace-nowrap p-4 ">
                        <h1 className="text-3xl font-semibold text-white text-center mb-8">Number of Messages Today </h1>
                        <table className="bg-white border-2 mx-auto">
                            <thead className="border">
                                <tr className="border">
                                    <th className="border px-2">No.</th>
                                    <th className="border px-2">Group Name</th>
                                    <th className="border px-2">Message Count</th>
                                </tr>
                            </thead>
                            <tbody>
                                {data.map(function (x) {
                                    console.log(x);
                                    i++;
                                    return (
                                        <tr className="border text-center" key={i}>
                                            <td className="border">{i}</td>
                                            <td className="border">{x.name}</td>
                                            <td className="border">{x.msg_count}</td>
                                        </tr>
                                    );
                                })}
                            </tbody>
                        </table>
                    </div>
                </div>
            </div>
        );
    }
}
